#include "param.h"
#include "types.h"
#include "riscv.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "defs.h"
#include "memlayout.h"

// kernel page table physical address
pagetable_t kernel_pagetable;

extern char etext[];  // kernel.ld sets this to end of kernel code.

extern char trampoline[]; // trampoline.S

// Make a direct-map page table for the kernel.
pagetable_t
kvmmake(void){
    pagetable_t kpgtable = (pagetable_t)kalloc();

    memset(kpgtable, 0, PGSIZE);

    kvmmap(kpgtable, PLIC, PLIC, (uint64)PLIC_MMAP_SIZE, PTE_R | PTE_W);

    kvmmap(kpgtable, UART0, UART0, PGSIZE, PTE_R | PTE_W);

    kvmmap(kpgtable, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

    kvmmap(kpgtable, KERNBASE, KERNBASE, (uint64)etext-KERNBASE, PTE_R | PTE_X);

    kvmmap(kpgtable, (uint64)etext, (uint64)etext, PHYSTOP - (uint64)etext, PTE_R | PTE_W);

    kvmmap(kpgtable, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);

    // set kernel stack for each proc
    proc_mapstacks(kpgtable);

    return kpgtable;
}

// create kernel page table and create mappings (ptes) from va to pa
void kvminit(){
    kernel_pagetable = kvmmake();
}

// Switch h/w page table register to the kernel's page table,
// and enable paging.
void
kvminithart()
{
    // wait for any previous writes to the page table memory to finish.
    sfence_vma();

    w_satp(MAKE_SATP(kernel_pagetable));

    // flush stale entries from the TLB.
    sfence_vma();
}

// Return the address of the PTE of leaf level page table from root page table `pgtable`
// that corresponds to virtual address `va`. 
// If alloc!=0,create any required page-table pages.
// <param>pgtable: phyiscal address of root page table </param>
// <param>va: virtual address to be mapped from </param>
pte_t* dowalk(pagetable_t pagetable, uint64 va, int alloc, int* count){
    if(va >= MAXVA)
        panic("walk");

    for(int level = 2; level > 0; level--) {
        pte_t *pte = &pagetable[PX(level, va)];
        if(*pte & PTE_V) {
            pagetable = (pagetable_t)PTE2PA(*pte);
        } else {
            if(!alloc || (pagetable = (pagetable_t)kalloc()) == 0)
                return 0;
      
            *count = (*count) + 1;
            memset(pagetable, 0, PGSIZE);
            *pte = PA2PTE(pagetable) | PTE_V;
        }
    }

    // pagetable is the leaf.
    return &pagetable[PX(0, va)];
}

pte_t * walk(pagetable_t pgtable, uint64 va, int alloc){
    int count = 0;
    return dowalk(pgtable, va, alloc, &count);
}

// map `va` to `pa` in 3-level pgtable
void kvmmap(pagetable_t pgtable, uint64 va, uint64 pa, uint64 size, int permission){
    if (mappages(pgtable, va, size, pa, permission) != 0){
        panic("mappages");
    }
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned. Returns 0 on success, -1 if walk() couldn't
// allocate a needed page-table page.
int
mappages(pagetable_t pgtable, uint64 va, uint64 size, uint64 pa, int permission){
    uint64 a, last;
    pte_t *pte;

    if(size == 0){
        panic("mappages: size");
    }

    a = PGROUNDDOWN(va);
    last = PGROUNDDOWN(va + size - 1);
    int total = 0;
    for (;;){ //; a <= last; a += PGSIZE, pa += PGSIZE
        int count = 0;
        if ((pte = dowalk(pgtable, a, 1, &count))==0){
            return -1;
        } //pte is the entry of leaf page table.

        if (*pte & PTE_V){
            panic("mappages");
        }
        // pte's ppn = ppn of pa
        *pte = PA2PTE(pa) | permission | PTE_V;
        if (a == last){
            break;
        }
        a += PGSIZE;
        pa += PGSIZE;
        total += count;
    }
    printf("%d page tables created.\n", total);
    return 0;
}

// create an empty user page table.
// returns 0 if out of memory.
pagetable_t uvmcreate()
{
    pagetable_t pagetable;
    pagetable = (pagetable_t) kalloc();
    if(pagetable == 0){
        return 0;
    }
    memset(pagetable, 0, PGSIZE);
    return pagetable;
}

// Load the user initcode into address 0 of pagetable,
// for the first process.
// sz must be less than a page.
void uvmfirst(pagetable_t pgtable, uchar *src, uint sz){
    char *mem;

    if(sz >= PGSIZE)
        panic("uvmfirst: more than a page");
    mem = kalloc();
    memset(mem, 0, PGSIZE);
    mappages(pgtable, 0, PGSIZE, (uint64)mem, PTE_W|PTE_R|PTE_X|PTE_U);
    memmove(mem, src, sz);
}

// copy each pte from old to new without actuall creating physical page for new pgtable.
void uvmcopy(pagetable_t old, pagetable_t new, uint64 size){
    
    for (int va = 0; va < size; va += PGSIZE){
        pte_t *pte = walk(old, va, 0);
        
        *pte &= ~PTE_W;
        *pte |= PTE_COW;

        int flag = PTE_FLAGS(*pte);
        uint64 pa = PTE2PA(*pte);
        if (mappages(new, va, PGSIZE, pa, flag) ){

        }
    }
}