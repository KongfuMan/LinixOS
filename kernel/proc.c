#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"

struct proc proc[NPROC];

void
proc_mapstacks(pagetable_t kpgtbl){
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