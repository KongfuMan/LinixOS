#include "param.h"
#include "types.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"
#include "memlayout.h"

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void trapinit(void)
{
//   initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void trapinithart(void)
{
    w_stvec((uint64)kernelvec);
}

void usertrapret(){
    
}
