// Physical memory allocator in unit of sinlge page
#include "param.h"
#include "types.h"
#include "riscv.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "defs.h"
#include "memlayout.h"

void kfreerange(void* pa_start, void* pa_end);
void kfree(void* pa);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run{
    struct run *next;
};

// manages a list of physical page frames
struct {
    //TODO: spin lock to protect
    struct run *freelist;
    int ref_count[PFCOUNT];
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
        panic("kfree");
    }

    memset(pa, 0, PGSIZE);

    r = (struct run*)pa;

    // TODO: acquire lock
    r->next = kmem.freelist;
    kmem.freelist = r;
    // TODO: release lock
}

// allocate a physical page and return the beginning addr
void*
kalloc(){
    struct run *r;
    //TODO: acquire lock
    r = kmem.freelist;
    if (r){
        kmem.freelist = r->next;
    }else{
        printf("Memory used up.");
    }
    
    //TODO: release lock

    if (r){
        memset((char*)r, 5, PGSIZE);
    }
    return (void*)r;
}