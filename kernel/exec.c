#include "types.h"

uint64 exec(void){
    // 1. fork current proc
    // 2. read binary to physical mem
    // 3. validate elf
    // 4. map pages from ua to pa
    // 5. set up user proc memory layout

    // uvmfirst(p->pagetable, initcode, sizeof(initcode));
    // p->sz = PGSIZE; //p->sz is the  size used memory of proc.

    // // 3. prepare for "sret" from kernel to user.
    // p->trapframe->epc = 0;       // user program counter
    // p->trapframe->sp  = PGSIZE;  // user stack pointer

    // safestrcpy(p->name, "initcode", sizeof(p->name));
    // // p->cwd = namei("/");

    // p->state = RUNNABLE;

    return 0;
}