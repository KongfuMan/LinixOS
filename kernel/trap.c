#include "param.h"
#include "types.h"
#include "riscv.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
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

void usertrap(){
    int cause = r_scause();
    if (cause == 0x80000000){
        
    }
}

void kerneltrap(){
    uint64 cause = r_scause();
    if (cause == 0x80000000){

    }
}

void usertrapret(){
    struct proc *p = current_proc();

    // we're about to switch the destination of traps from
    // kerneltrap() to usertrap(), so turn off interrupts until
    // we're back in user space, where usertrap() is correct.
    // intr_off();

    // send syscalls, interrupts, and exceptions to uservec in trampoline.S

    // set up trapframe values that uservec will need when
    // the process next traps into the kernel.

    // set up the registers that trampoline.S's sret will use
    // to get to user space.

    // set S Previous Privilege mode to User.

    // set S Exception Program Counter to the saved user pc.
    w_sepc(p->trapframe->epc);

    // tell trampoline.S the user page table to switch to.

    // jump to userret in trampoline.S at the top of memory, which 
    // switches to the user page table, restores user registers,
    // and switches to user mode with sret.
}
