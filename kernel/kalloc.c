// Physical memory allocator in unit of sinlge page
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

void kfreerange(void* pa_start, void* pa_end);
void kfree(void* pa);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run{
    struct run *next;
};

// manages a list of physical page frames
struct {
    struct spinlock lock;
    struct run *freelist;
    // ref count of each physical page frame.
    // one physical page can be mapped to virtual COW page of various processes.
    int ref_count[PFCOUNT]; 
}kmem;

void
kinit()
{
    initlock(&kmem.lock, "kmem");
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

    if (get_ref(pa) != 0){
        panic("kfree: cannot free a physical page with non-zero ref count");
    }

    memset(pa, 0, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
}

// allocate a physical page and return the beginning addr
void*
kalloc(){
    struct run *r;
    acquire(&kmem.lock);
    r = kmem.freelist;
    if (r){
        kmem.freelist = r->next;
    }else{
        printf("Memory used up.");
    }
    release(&kmem.lock);

    if (r){
        memset((char*)r, 5, PGSIZE);
    }
    return (void*)r;
}

int get_ref(void *pa){
    int pfn = PA2PFN(pa);
    if (pfn < 0 || pfn >= PFCOUNT){
        panic("get_ref pfn out of range");
    }
    return kmem.ref_count[pfn];
}

void incr_ref(void *pa){
    int pfn = PA2PFN(pa);
    if (pfn < 0 || pfn >= PFCOUNT){
        panic("get_ref pfn out of range");
    }
    acquire(&kmem.lock);
    ++kmem.ref_count[pfn];
    release(&kmem.lock);
}

void decr_ref(void *pa){
    int pfn = PA2PFN(pa);
    if (pfn < 0 || pfn >= PFCOUNT){
        panic("get_ref pfn out of range");
    }
    acquire(&kmem.lock);
    --kmem.ref_count[pfn];
    release(&kmem.lock);
}