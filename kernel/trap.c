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

// to protect ticks.
struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void trapinithart(void)
{
    w_stvec((uint64)kernelvec);
}

/*
This routine handles page fault. It determines the address,
and the problem, and then pass it off to the appropriate routines 
*/
void do_page_fault(){
    // cases: 1. store/load invlid page:
    //            a. find a free page to use
    //            b. no free page, swap out one and reuse
    //        2. store cow page
    //        3. store/load page that was swapped out before.
    //        4.
    uint64 stval = r_stval();
    uint64 scause = r_scause();
    printf("do page fault.\n");
}

void usertrap(){
    int intr = intr_get();
    if (intr){
        panic("usertrap: interrupt is enabled.");
    }

    if((r_sstatus() & SSTATUS_SPP) != 0){
        panic("usertrap: not from user mode");
    }

    w_stvec((uint64)kernelvec);

    struct proc *p = current_proc();

    // save user program counter.
    p->trapframe->epc = r_sepc();

    uint64 scause = r_scause();
    int which_dev = 0;
    if(scause == 8){
        // ecall from U-mode
        
        p->trapframe->epc += 4;

        intr_on();
        syscall();
    }else if(scause == 13 || scause == 15 || scause == 5 || scause == 7){
        // loast/store access/page fault
        do_page_fault();
    }else if ((which_dev = devintr()) != 0){
        // known unhandled source

    }else{
        printf("unhandled ex");
    }
    
    usertrapret();
}

void kerneltrap(){
    int which_dev = 0;
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus();
    // uint64 scause = r_scause();

    if((which_dev = devintr()) == 0){
        //unknown
    }

    if (which_dev == 2){
        // timer interrupt
    }

    // the yield() may have caused some traps to occur,
    // so restore trap registers for use by kernelvec.S's sepc instruction.
    w_sepc(sepc);
    w_sstatus(sstatus);
}

int devintr(){
    uint64 scause = r_scause();
    if((scause & 0x8000000000000000L) != 0 && (scause & 0xff) == 9){
        // S-mode external interrupt via PLIC
        // irq indicates which device interrupted.
        int irq = plic_claim();

        if(irq == UART0_IRQ){
            // printf("Uart device intr. \n");
        } else if(irq == VIRTIO0_IRQ){
            virtio_disk_intr();
        } else if(irq){
            printf("unexpected interrupt irq=%d\n", irq);
        }

        // the PLIC allows each device to raise at most one
        // interrupt at a time; tell the PLIC the device is
        // now allowed to interrupt again.
        if(irq){
            plic_complete(irq);
        }

        return 1;
    }

    if(scause == 0x8000000000000001L){
        // printf("S-mode software interrupt from M-mode timervec. \n");
        // software interrupt from a machine-mode timer interrupt,
        // forwarded by timervec in kernelvec.S.

        // if(cpuid() == 0){
        //     clockintr();
        // }

        acquire(&tickslock);
        ticks++;
        release(&tickslock);

        // acknowledge the software interrupt by clearing
        // the SSIP bit in sip.
        // printf("timer interrupt. ticks: %d.\n", ticks);
        w_sip(r_sip() & ~2);
        return 2;
    }

    return 0;
}

//
// return to user space
//
void
usertrapret(void)
{
    struct proc *p = current_proc();

    // we're about to switch the destination of traps from
    // kerneltrap() to usertrap(), so turn off interrupts until
    // we're back in user space, where usertrap() is correct.
    intr_off();

    // send syscalls, interrupts, and exceptions to uservec in trampoline.S
    uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
    w_stvec(trampoline_uservec);

    // set up trapframe values that uservec will need when
    // the process next traps into the kernel.
    p->trapframe->kernel_satp = r_satp();         // kernel page table
    p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
    p->trapframe->kernel_trap = (uint64)usertrap;
    p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

    // set up the registers that trampoline.S's sret will use
    // to get to user space.

    // set S Previous Privilege mode to User.
    unsigned long x = r_sstatus();
    x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
    x |= SSTATUS_SPIE; // enable interrupts in user mode
    w_sstatus(x);

    // set S Exception Program Counter to the saved user pc.
    w_sepc(p->trapframe->epc);

    // tell trampoline.S the user page table to switch to.
    uint64 satp = MAKE_SATP(p->pagetable);

    // jump to userret in trampoline.S at the top of memory, which 
    // switches to the user page table, restores user registers,
    // and switches to user mode with sret.
    uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
    ((void (*)(uint64))trampoline_userret)(satp);
}