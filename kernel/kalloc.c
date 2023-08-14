// Physical memory allocator with minal granularity of a page
// 

#include "defs.h"
#include "memlayout.h"
#include "riscv.h"
#include "types.h"

void kfreerange(void* pa_start, void* pa_end);
void kfree(void* pa);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run{
    struct run *next;
};

struct {
    //TODO: spin lock to protect
    struct run *freelist;
}kmem;

void
kinit()
{
    // TODO: init kmem->spinlock

    kfreerange(end, (void*)PHYSTOP);
}

// free the contiguous physical pages start from 
// PGROUNDUP(pa_start) to PGROUNDDOWN(pa_end)
void kfreerange(void* pa_start, void* pa_end){
    char *pa;
    pa = (char*)PGROUNDUP((uint64)pa_start);
    for (; pa + PGSIZE <= (char*)pa_end; pa += PGSIZE){
        kfree(pa);
    }
}

// free a physical page with beginning addr of `pa`
void
kfree(void* pa){
    struct run* r;
    if ((uint64)pa % PGSIZE != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP){
        // panic
        return;
    }

    memset(pa, 0, PGSIZE);

    r = (struct run*)pa;

    //TODO: acquire lock
    r->next = kmem.freelist;
    kmem.freelist = r;
    //TODO: release lock
}

// allocate a physical page and return the beginning addr
void*
kalloc(){
    struct run *r;
    //TODO: acquire lock
    r = kmem.freelist;
    if (r){
        kmem.freelist = r->next;
    }
    //TODO: release lock

    if (r){
        memset((char*)r, 1, PGSIZE);
    }
    return (void*)r;
}