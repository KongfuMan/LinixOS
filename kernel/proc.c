#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"

extern char trampoline[]; // trampoline.S

struct cpu cpus[NCPU];
struct proc proc[NPROC];

struct proc *initproc;

int cpuid(void){
    return r_tp();
}

struct cpu* current_cpu(){
    int hart = cpuid();
    if (hart >= NCPU){
        return 0;
    }
    return &cpus[hart];
}

struct proc* current_proc(){
    struct cpu* cpu = current_cpu();
    if (cpu){
        return cpu->proc;
    }
    return 0;
}

// Per-CPU scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void scheduler(void){
    struct proc *p;
    struct cpu *c = current_cpu();
    while(1){
        for (p = proc; p < &proc[NPROC]; p++){
            if (p->state == RUNNABLE){
                p->state = RUNNING;
                // `p` will be switched to run on `c`
                c->proc = p;

                swtch(&c->context, &p->context);

                // switched back to scheduler, reset `c->proc` to `null`
                c->proc = 0;
            }
        }
    }
}

// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

void proc_mapstacks(pagetable_t kpgtbl){
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

void forkret(){
    printf("running forkret. \n");
    usertrapret();
}

// init all the proc
void procinit(){
    struct proc *p;

    // TODO: lock
    for(p = proc; p < &proc[NPROC]; p++) {
        // initlock(&p->lock, "proc");
        p->state = UNUSED;
        p->kstack = KSTACK((int) (p - proc));
    }
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct proc *p){

    pagetable_t pgtable = uvmcreate();
    if (pgtable == 0){
        return 0;
    }
    
    // map user trampoline
    if(mappages(pgtable, TRAMPOLINE, PGSIZE, (uint64)trampoline, PTE_R | PTE_X) < 0){
        // TODO: remove ptes that might have already been created
        return 0;
    }

    // map user trapframe
    if (mappages(pgtable, TRAPFRAME, PGSIZE, (uint64)p->trapframe, PTE_R | PTE_W) < 0){
        // TODO: 
        return 0;
    }
    p->trapframe = (struct trapframe*)TRAPFRAME;
    return pgtable;
}

struct proc* allocproc(){
    struct proc *p;
    for (p = proc; p < &proc[NPROC]; p++){
        // TODO: acquire lock
        if (p->state == UNUSED){
            goto found;
        }else{
            // TODO: release lock
        }
    }

    printf("Cannot find an available unused proc. \n");
    return 0;

found:
    // init the proc just found
    p->pid = ((uint64)(p - proc) / sizeof(struct proc)); // TODO: allocpid();
    p->state = USED;

    // Allocate a trapframe page.
    if ((p->trapframe = (struct trapframe*)kalloc()) == 0){
        // TODO: freeproc(p);
        // release(&p->lock);
        return 0;
    }

    // An empty user page table.
    p->pagetable = proc_pagetable(p);
    if (p->pagetable == 0){
        // TODO: freeproc(p);
        // release(&p->lock);
        return 0;
    }

    // Set up new context to start executing at forkret,
    // which returns to user space.
    memset(&p->context, 0, sizeof(struct context));
    p->context.ra = (uint64)forkret;
    p->context.sp = p->kstack + PGSIZE; //use kernel to 
    return p;
}

// Create the first proc, which runs the initcode
void userinit(void){
    // 1. alloc a unnused proc
    struct proc *p = allocproc();
    if (!p){
        panic("userinit");
    }
    initproc = p;

    // 2. allocate one user page and copy initcode's instructions and data into it.
    // 

    // 3. prepare for the very first "sret" from kernel to user.

    p->state = RUNNABLE;
}