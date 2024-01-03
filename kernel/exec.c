#include "param.h"
#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "net.h"
#include "defs.h"
#include "file.h"
#include "stat.h"
#include "elf.h"

static int loadseg(pde_t *, uint64, struct inode *, uint, uint);

int flags2perm(int flags)
{
    int perm = 0;
    if(flags & 0x1){
        perm = PTE_X;
    }
    if(flags & 0x2){
        perm |= PTE_W;
    }
    return perm;
}

// path of executable and array of arguments
int exec(char *path, char **argv){
    int i, off;
    char *s, *last;
    uint64 sz = 0, sp, stackbase, argc, ustack[MAXARG];
    struct inode *ip;
    struct elfhdr elf;
    struct proghdr ph;
    pagetable_t pgtable, oldpagetable;
    struct proc *p = current_proc();

    // 1. find the inode by path in fs namespace
    if((ip = namei(path)) == 0){
        return -1;
    }
    ilock(ip);

    // 2. validate elf of binary image
    if (readi(ip, 0, (uint64)&elf, 0, sizeof(elf)) != sizeof(elf)){
        panic("exec::readi");
    }
    if (elf.magic != ELF_MAGIC){
        goto bad;
    }
 
    // create an new pagetable with trampoline program mapped
    // and TRAMFRAME of new pgtable is mapped to trampframe of currrent proc
    if(!(pgtable = proc_pagetable(p))){
        goto bad;
    }

    // 3. load all binary
    for(i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph)){
        // load all sections in the program header table
        if(readi(ip, 0, (uint64)&ph, off, sizeof(ph)) != sizeof(ph)){
            goto bad;
        }
        if(ph.type != ELF_PROG_LOAD){
            continue;
        }

        // a series parameters check
        if(ph.memsz < ph.filesz || ph.vaddr + ph.memsz < ph.vaddr || ph.vaddr % PGSIZE != 0){
            goto bad;
        }

        // load program header entry into user virtual address space 
        uint64 sz1;
        if((sz1 = uvmalloc(pgtable, sz, ph.vaddr + ph.memsz, flags2perm(ph.flags))) == 0){
            goto bad;
        }
        sz = sz1;
        loadseg(pgtable, ph.vaddr, ip, ph.off, ph.filesz);
    }

    iunlockput(ip);

    // prepare stack for elf entry function
    ip = 0;
    p = current_proc();
    uint64 oldsz = p->sz;

    // Allocate two pages at the next page boundary.
    // Make the first inaccessible as a stack guard.
    // Use the second as the user stack.
    sz = PGROUNDUP(sz);
    uint64 sz1;
    if ((sz1 = uvmalloc(pgtable, sz, sz + 2*PGSIZE, PTE_W)) == 0){
        goto bad;
    }
    sz = sz1;
    uvmclear(pgtable, sz-2*PGSIZE);
    sp = sz;
    stackbase = sp - PGSIZE;

    // Push argument strings, prepare rest of stack in ustack.
    for(argc = 0; argv[argc] != 0; argc++) {
        if(argc >= MAXARG){
            goto bad;
        }
        sp -= strlen(argv[argc]) + 1;
        sp -= sp % 16; // riscv sp must be 16-byte aligned
        if(sp < stackbase){
            goto bad;
        }
        if(copyout(pgtable, sp, argv[argc], strlen(argv[argc]) + 1) < 0){
            goto bad;
        }
        ustack[argc] = sp;
    }
    ustack[argc] = 0;

    // push the array of argv[] pointers.
    sp -= (argc+1) * sizeof(uint64);
    sp -= sp % 16;
    if(sp < stackbase){
        goto bad;
    }
    if(copyout(pgtable, sp, (char *)ustack, (argc+1)*sizeof(uint64)) < 0){
        goto bad;
    }

    // arguments to user main(argc, argv)
    // argc is returned via the system call return
    // value, which goes in a0.
    p->trapframe->a1 = sp;

    // Save program name for debugging.
    for(last=s=path; *s; s++){
        if(*s == '/'){
            last = s+1;
        }
    }
    safestrcpy(p->name, last, sizeof(p->name));

    // Commit to the user image.
    oldpagetable = p->pagetable;
    p->pagetable = pgtable;
    p->sz = sz;
    p->trapframe->epc = elf.entry;  // initial program counter = main
    p->trapframe->sp = sp;          // initial stack pointer
    proc_freepagetable(oldpagetable, oldsz);

    return argc; // this ends up in a0, the first argument to main(argc, argv)
bad:
    if(pgtable){
        proc_freepagetable(pgtable, sz);
    }
    if(ip){
        iunlockput(ip);
        // TODO: end_op();
    }
    return -1;
}

// Load a program segment at virtual address `uva` into `pagetable` 
// from the disk blocks indexed by inode.
// va must be page-aligned
// and the pages from uva to uva+sz must already be mapped.
// Returns 0 on success, -1 on failure.
static int
loadseg(pagetable_t pagetable, uint64 uva, struct inode *ip, uint offset, uint sz){
    
    uint64 uva0, pa0, kva0;
    int n;
    for (uva0 = uva; uva0 < uva + sz; uva0 += PGSIZE){
        if ((pa0 = walkaddr(pagetable, uva0)) == 0){
            return -1;
        }

        kva0 = pa0;
        n = min(sz - (uva0 - uva), PGSIZE);

        if (readi(ip, 0, kva0, offset, n) != n){
            return -1;
        }
        offset += n;
    }

    return 0;
}