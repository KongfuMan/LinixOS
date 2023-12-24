#include "param.h"
#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
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
void
kvminit(){
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
pte_t*
dowalk(pagetable_t pagetable, uint64 va, int alloc, int* count){
    if(va >= MAXVA)
        panic("walk");

    for(int level = 2; level > 0; level--) {
        pte_t *pte = &pagetable[PX(level, va)];
        if(*pte & PTE_V) {
            pagetable = (pagetable_t)PTE2PA(*pte);
        } else {
            if(!alloc || (pagetable = (pagetable_t)kalloc()) == 0)
                return 0;
            // count of new pte created for debug purpose
            *count = (*count) + 1;
            memset(pagetable, 0, PGSIZE);
            *pte = PA2PTE(pagetable) | PTE_V;
        }
    }

    // pagetable is the leaf.
    return &pagetable[PX(0, va)];
}

pte_t*
walk(pagetable_t pgtable, uint64 va, int alloc){
    int count = 0;
    return dowalk(pgtable, va, alloc, &count);
}

// Look up a virtual address `va`, return the physical address,
// or 0 if not yet mapped in page table.
// Can only be used to look up user pages.
uint64
walkaddr(pagetable_t pagetable, uint64 va){
    pte_t *pte;
    uint64 pa;

    if(va >= MAXVA)
        return 0;

    pte = walk(pagetable, va, 0);
    if(pte == 0)
        return 0;

    if((*pte & PTE_V) == 0)
        return 0;

    if((*pte & PTE_U) == 0)
        return 0;

    pa = PTE2PA(*pte);
    return pa;
}

// map `va` to `pa` in 3-level pgtable
void
kvmmap(pagetable_t pgtable, uint64 va, uint64 pa, uint64 size, int permission){
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

    if (total){
        printf("%d page tables created.\n", total);
    }
    return 0;
}

// unmap `n` pages starting from `va`.
// do_free indiates whether free the physical page.
void
uvmunmap(pagetable_t pgtable, uint64 va, int n, int do_free){
    if (va % PGSIZE){
        panic("uvmunmap: given va not page aligned.");
    }
    uint64 addr;
    pte_t *pte;
    for (addr = va; (addr-va) / PGSIZE < n; addr += PGSIZE){
        if ((pte = walk(pgtable, addr, 0)) == 0){
            panic("uvmunmap");
        }
        if (!((*pte) & PTE_V)){
            panic("uvmunmap: page not mapped.");
        }
        if (PTE_FLAGS(*pte) == PTE_V){
            panic("uvmunmap: page is not leaf.");
        }
        if (do_free){
            kfree((void*)PTE2PA(*pte));
        }
        *pte = 0; //set pte to null
    }
}

// create an empty user page table.
// returns 0 if out of memory.
pagetable_t
uvmcreate()
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
void
uvmfirst(pagetable_t pgtable, uchar *src, uint sz){
    char *mem;

    if(sz >= PGSIZE)
        panic("uvmfirst: more than a page");
    mem = kalloc();
    memset(mem, 0, PGSIZE);
    mappages(pgtable, 0, PGSIZE, (uint64)mem, PTE_W|PTE_R|PTE_X|PTE_U);
    memmove(mem, src, sz);
}

// Allocate PTEs and physical memory to grow process from old size to
// new size, which need not be page aligned.  Returns new size or 0 on error.
uint64
uvmalloc(pagetable_t pagetable, uint64 oldsize, uint64 newsize, int xperm)
{
    if(newsize < oldsize){
        return oldsize;
    }

    char *pa0;
    uint64 uva0;
    uint64 oldsize1 = PGROUNDUP(oldsize);
    for (uva0 = oldsize1; uva0 < newsize; uva0 += PGSIZE){
        pa0 = kalloc();
        if (!pa0){
            uvmdealloc(pagetable, uva0, oldsize);
            return 0;
        }
        memset((void*)pa0, 0, PGSIZE);
        if (mappages(pagetable, uva0, PGSIZE, (uint64)pa0, PTE_R | PTE_U | xperm) != 0){
            uvmdealloc(pagetable, uva0, oldsize);
            return 0;
        }
    }

    return newsize;
}

// Deallocate user pages to bring the process size from oldsize to
// newsize.  oldsize and newsize need not be page-aligned, nor does newsize
// need to be less than oldsize.  
// oldsize can be larger than the actual process size.  
// Returns the new process size.
uint64
uvmdealloc(pagetable_t pagetable, uint64 oldsize, uint64 newsize){
    if(newsize >= oldsize){
        return oldsize;
    }

    if(PGROUNDUP(newsize) < PGROUNDUP(oldsize)){
        int npages = (PGROUNDUP(oldsize) - PGROUNDUP(newsize)) / PGSIZE;
        uvmunmap(pagetable, PGROUNDUP(newsize), npages, 1);
    }

    return newsize;
}

// copy each pte from old to new without actually allocating physical page for new pgtable.
// retrun 0 on success, -1 on failure
// int
// uvmcopy(pagetable_t old, pagetable_t new, uint64 size){
//     int va = 0;
//     for (; va < size; va += PGSIZE){
//         pte_t *pte = walk(old, va, 0);
        
//         *pte &= ~PTE_W;
//         *pte |= PTE_COW;

//         int flag = PTE_FLAGS(*pte);
//         uint64 pa = PTE2PA(*pte);
//         incr_ref((void*)pa);
//         if (mappages(new, va, PGSIZE, pa, flag) != 0){
//             goto err;
//         }
//     }
//     return 0;

// err:
//     uvmunmap(new, 0, va / PGSIZE, 0);
//     return -1;
// }

int
uvmcopy(pagetable_t old, pagetable_t new, uint64 size){
    int va = 0;
    uint64 oldpa;
    uint64 newpa;
    pte_t *pte;
    for (; va < size; va += PGSIZE){
        if((pte = walk(old, va, 0)) == 0){
            panic("uvmcopy: pte should exist");
        }
        
        if (((*pte) & PTE_V) == 0){
            panic("uvmcopy: page not present");
        }

        if ((newpa = (uint64)kalloc()) == 0){
            goto err;
        }

        uint64 oldpa = PTE2PA(*pte);
        int flag = PTE_FLAGS(*pte);
        memmove((void *)newpa, (void *)oldpa, PGSIZE);
        if (mappages(new, va, PGSIZE, newpa, flag) != 0){
            goto err;
        }
    }
    return 0;

err:
    uvmunmap(new, 0, va / PGSIZE, 0);
    return -1;
}

// mark a PTE invalid for user access.
// used by exec for the user stack guard page.
void
uvmclear(pagetable_t pagetable, uint64 va){
    pte_t *pte;
    if((pte = walk(pagetable, va, 0)) == 0){
        panic("uvmclear");
    }

    *pte &= ~PTE_U;
}

// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void
freewalk(pagetable_t pagetable)
{
    // there are 2^9 = 512 PTEs in a page table.
    for(int i = 0; i < 512; i++){
        pte_t pte = pagetable[i];
        if((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0){
            // this PTE points to a lower-level page table.
            uint64 child = PTE2PA(pte);
            freewalk((pagetable_t)child);
            pagetable[i] = 0;
        } else if(pte & PTE_V){
            panic("freewalk: leaf");
        }
    }
    kfree((void*)pagetable);
}

// Free user memory pages,
// then free page-table pages.
void
uvmfree(pagetable_t pagetable, uint64 sz)
{
    if(sz > 0){
        uvmunmap(pagetable, 0, PGROUNDUP(sz)/PGSIZE, 1);
    }
    freewalk(pagetable);
}

// Copy from kernel to proc user space.
// Copy `len` bytes starting from kernel `src` to user `dstva` in a given page table.
// Return 0 on success, -1 on error.
int
copyout(pagetable_t pagetable, uint64 dst_uva, char *src_kva, uint64 len){
    uint64 size = 0, va0 = 0, pa0 = 0;
    while(len > 0){
        va0 = PGROUNDDOWN(dst_uva);
        pa0 = walkaddr(pagetable, va0);   // same as the kernel va

        if (!pa0){
            return -1;
        }

        size = min(PGSIZE - (dst_uva - va0), len); // n is actual size copied
        memmove((void *)(pa0 + (dst_uva - va0)), src_kva, size);
        len -= size;
        src_kva += size;
        dst_uva = PGSIZE + va0; // next page
    }

    return 0;
}

// Copy from proc user space to kernel.
// Copy `len` bytes starting from user `src` to kernel `dstva` in a given page table.
// Return 0 on success, -1 on error.
int
copyin(pagetable_t pagetable, char *dst_kva, uint64 src_uva, uint64 len){
    uint64 src_uva0, src_p0, src_kva0, size;
    while(len > 0){
        src_uva0 = PGROUNDDOWN(src_uva);
        src_p0 = walkaddr(pagetable, src_uva0);

        if (!src_p0){
            return -1;
        }

        src_kva0 = src_p0;
        size = min(len, PGSIZE - (src_uva - src_uva0));
        memmove((void*)dst_kva, (void*)(src_kva0 + (src_uva - src_uva0)), size);
        len -= size;
        src_uva = PGSIZE + src_uva0; // next page
        dst_kva += size;
    }

    return 0;
}

// Copy a null-terminated string from user to kernel.
// until a '\0', or max.
// Return 0 on success, -1 on error.
int
copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
    uint64 n, va0, pa0;
    int got_null = 0;

    while(got_null == 0 && max > 0){
        va0 = PGROUNDDOWN(srcva);
        pa0 = walkaddr(pagetable, va0);
        if(pa0 == 0)
        return -1;
        n = PGSIZE - (srcva - va0);
        if(n > max)
        n = max;

        char *p = (char *) (pa0 + (srcva - va0));
        while(n > 0){
            if(*p == '\0'){
                *dst = '\0';
                got_null = 1;
                break;
            } else {
                *dst = *p;
            }
            --n;
            --max;
            p++;
            dst++;
        }

        srcva = va0 + PGSIZE;
    }

    if(got_null){
        return 0;
    }
    return -1;
}
