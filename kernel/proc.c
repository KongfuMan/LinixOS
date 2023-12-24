#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "defs.h"
#include "memlayout.h"

extern char trampoline[]; // trampoline.S

struct cpu cpus[NCPU];
struct proc proc[NPROC];

struct proc *initproc;

static void freeproc(struct proc *p);

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

int cpuid(void){
    return r_tp();
}

struct cpu* current_cpu(){
    int hart = cpuid();
    if (hart >= NCPU){
        return 0;
    }
    return &cpus[hart];
}

struct proc* current_proc(){
    struct cpu* cpu = current_cpu();
    if (cpu){
        return cpu->proc;
    }
    return 0;
}

// Per-CPU scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void scheduler(){
    struct proc *p;
    struct cpu *c = current_cpu();
    while(1){
        int intrstatus = intr_get();
        if (!intrstatus){
            intr_on();
        }
        for (p = proc; p < &proc[NPROC]; p++){
            acquire(&p->lock);
            if (p->state == RUNNABLE){
                p->state = RUNNING;
                // `p` will be switched to run on `c`
                c->proc = p;

                swtch(&c->context, &p->context);

                // switched back to scheduler, reset `c->proc` to `null`
                c->proc = 0;
            }
            release(&p->lock);
        }
    }
}

void proc_mapstacks(pagetable_t kpgtbl){
    struct proc *p;

    for(p = proc; p < &proc[NPROC]; p++){
        char *pa = kalloc();
        if (!pa){
            panic("kalloc");
        }
        uint64 va = KSTACK((int)(p - proc));
        kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
    }
}

static int first_proc = 1;

void forkret(){
    // release because current_proc()->lock was held before context switch in scheduler() func
    release(&current_proc()->lock);

    // init file system
    if (first_proc == 1){
        fsinit(ROOTDEV);
        first_proc = 0;
    }

    usertrapret();
}

// init all the proc
void procinit(){
    struct proc *p;

    // TODO: lock
    for(p = proc; p < &proc[NPROC]; p++) {
        initlock(&p->lock, "proc");
        p->state = UNUSED;
        p->kstack = KSTACK((int) (p - proc));
    }
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct proc *p){

    pagetable_t pgtable = uvmcreate();
    if (pgtable == 0){
        return 0;
    }
    
    // map user trampoline
    if(mappages(pgtable, TRAMPOLINE, PGSIZE, (uint64)trampoline, PTE_R | PTE_X) < 0){
        // TODO: roll back ptes that have already been created
        return 0;
    }

    // map user trapframe
    if (mappages(pgtable, TRAPFRAME, PGSIZE, (uint64)p->trapframe, PTE_R | PTE_W) < 0){
        // TODO: roll back ptes that have already been created
        return 0;
    }
    
    return pgtable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmunmap(pagetable, TRAPFRAME, 1, 0);
    uvmfree(pagetable, sz);
}

// allocate an empty proc and do necessary initializatio
// - set proc id
// - set proc state
// - create proc user page table
// - map TRAPFRAME to the pa of tramframe
// - 
struct proc* allocproc(){
    struct proc *p;
    for (p = proc; p < &proc[NPROC]; p++){
        acquire(&p->lock);
        if (p->state == UNUSED){
            goto found;
        }else{
            release(&p->lock);
        }
    }

    printf("Cannot find an available unused proc. \n");
    return 0;

found:
    // init the proc just found
    p->pid = (pid_t)(p - proc); // TODO: allocpid();
    p->state = USED;

    // Allocate a trapframe page.
    if ((p->trapframe = (struct trapframe*)kalloc()) == 0){
        // TODO: freeproc(p);
        release(&p->lock);
        return 0;
    }

    // An empty user page table.
    p->pagetable = proc_pagetable(p);
    if (p->pagetable == 0){
        // TODO: freeproc(p);
        release(&p->lock);
        return 0;
    }

    // Set up new context to start executing at forkret,
    // which returns to user space.
    memset(&p->context, 0, sizeof(struct context));
    p->context.ra = (uint64)forkret;
    p->context.sp = p->kstack + PGSIZE; //use kernel to 
    return p;
}

// a user program that calls exec("/init")
// assembled from ../user/initcode.S by executing command:
// od -t xC user/initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Create the first proc, which runs the initcode above.
void userinit(void){
    // 1. alloc a unnused proc
    struct proc *p = allocproc();
    if (!p){
        panic("userinit");
    }
    initproc = p;

    // 2. allocate one user page and copy initcode's instructions and data into it.
    uvmfirst(p->pagetable, initcode, sizeof(initcode));
    p->sz = PGSIZE; //p->sz is the  size used memory of proc.

    // 3. prepare for the very first "sret" from kernel to user.
    p->trapframe->epc = 0;       // user program counter
    p->trapframe->sp  = PGSIZE;  // user stack pointer

    safestrcpy(p->name, "initcode", sizeof(p->name));
    p->cwd = namei("/");

    p->state = RUNNABLE;

   release(&p->lock);
}

// current proc yield and switch to scheduler.
// sechduler will find next runnable proc to context switch
void sched(){
    int intena;
    struct proc* p = current_proc();

    if(!holding(&p->lock))
        panic("sched p->lock");
    
    if (current_cpu()->noff != 1) // interupt turned off for only once
        panic("sched locks");

    if(p->state == RUNNING)
        panic("sched running");

    if(intr_get())
        panic("sched interruptible");

    intena = current_cpu()->intena;   // intr will be enabled in scheduler, so should restore after swtch return.
    swtch(&p->context, &current_cpu()->context);
    current_cpu()->intena = intena;
}

void yield(void){
    struct proc *p = current_proc();
    p->state = SLEEPING;
    sched();
}

void sleep(void* chan, struct spinlock *lk){
    struct proc *p = current_proc();

    acquire(&p->lock);
    release(lk);
    p->chan = chan;
    p->state = SLEEPING;
    sched();
    p->state = RUNNABLE;
    p->chan = 0;
    release(&p->lock);
    acquire(lk);
}

// fork a child proc return child proc id if success,
// return -1 otherwise
pid_t fork(){
    struct proc *np, *p;
    int i, pid;
    p = current_proc();
    np = allocproc();

    if (!np){
        return -1;
    }

    // copy page table, change PTE_W -> PTE_COW
    if (uvmcopy(p->pagetable, np->pagetable, p->sz) == -1){
        // free np
        return -1;
    }
    np->sz = p->sz;

    // copy saved user registers.
    *(np->trapframe) = *(p->trapframe);

    // Cause fork to return 0 in the child.
    np->trapframe->a0 = 0;

    // increment reference counts on open file descriptors.
    for(i = 0; i < NOFILE; i++){
        if(p->ofile[i]){
            np->ofile[i] = filedup(p->ofile[i]);
        }
    }

    np->cwd = idup(p->cwd);

    safestrcpy(np->name, p->name, sizeof(p->name));

    pid = np->pid;

    release(&np->lock);

    acquire(&wait_lock);
    np->parent = p;
    release(&wait_lock);

    acquire(&np->lock);
    np->state = RUNNABLE;
    release(&np->lock);

    return pid;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p){
    if(p->trapframe){
        kfree((void*)p->trapframe);
    }
    p->trapframe = 0;
    if(p->pagetable){
        proc_freepagetable(p->pagetable, p->sz);
    }
    p->pagetable = 0;
    p->sz = 0;
    p->pid = 0;
    p->parent = 0;
    p->name[0] = 0;
    p->chan = 0;
    p->killed = 0;
    p->xstate = 0;
    p->state = UNUSED;
}

// wake up all sleeping proc
void wakeup(void* chan){
    struct proc *p;
    for (p = proc; p < proc + NPROC; p++){
        acquire(&p->lock);
        if (p->chan == chan){
            p->state = RUNNABLE;
        }
        release(&p->lock);
    }
}

// copy kernel to either a uva (usr_dst==1), or kva(usr_dst==0)
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len){
    if (user_dst){
        // dst is uva
        return copyout(current_proc()->pagetable, dst, (char*)src, len);
    }

    // dst is kva
    memmove((char *)dst, src, len);
    return 0;
}

// copy into kernel space from either a uva (usr_dst==1), or kva(usr_dst==0)
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len){
    if (user_src){
        // src is uva
        return copyin(current_proc()->pagetable, (char *)dst, src, len);
    }
    // src is kva
    memmove(dst, (void *)src, len);
    return 0;
}

// Wait for a child process to exit and copy the status to addr and return its pid
// Return -1 if this process has no children.
int
wait(uint64 addr){
    struct proc *p;
    int haschild, pid;
    struct proc *pp = current_proc();
    // Find all children and check any killed?
    // If no ZOMBIE child, sleep on the wait_lock and 
    // wait for any child call exit to wake up it.
    // Copyout xstate to addr and return child procid

    acquire(&wait_lock); // wait lock to protect pp

    while (1){
        haschild = 0;
        for (p = proc; p < proc + NPROC; p++){
            acquire(&p->lock);
            if (p->parent == pp){
                haschild = 1;
                if (p->state == ZOMBIE){
                    pid = p->pid;
                    if (addr != 0 && 
                        copyout(pp->pagetable, addr, (char *)&p->xstate, sizeof(p->xstate)) != 0){
                        release(&p->lock);
                        release(&wait_lock);
                        return -1;
                    }
                    freeproc(p);
                    release(&p->lock);
                    release(&wait_lock);
                    return pid;
                }
            }
            
            release(&p->lock);
        }

        if (!haschild){
            release(&wait_lock);
            return -1;
        }

        sleep(pp, &wait_lock);
    }
}

void
exit(int status){
    int fd;
    struct proc *p = current_proc();
    if (p == initproc){
        panic("initproc exit. \n");
    }

    // Close all open files
    for (fd = 0; fd < NFILE; fd++){
        if (p->ofile[fd]){
            fileclose(p->ofile[fd]);
            p->ofile[fd] = 0;
        }
    }

    acquire(&wait_lock);

    // Give any children to init.
    // TODO: reparent(p);

    wakeup(p->parent);
    
    acquire(&p->lock);
    p->xstate = p->state;
    p->state = ZOMBIE;
    release(&wait_lock);

    sched();
    panic("exit");
}

