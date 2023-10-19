#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
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
        int intrstatus = intr_get();
        if (!intrstatus){
            intr_on();
        }
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

static int first_proc = 1;

void forkret(){
    //init file system
    if (first_proc == 1){
        fsinit(ROOTDEV);
        first_proc = 0;
    }

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
    
    return pgtable;
}

// allocate an empty proc and do necessary initializatio
// - set proc id
// - set proc state
// - create proc user page table
// - map TRAPFRAME to the pa of tramframe
// - 
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

// a user program that calls exec("/init")
// assembled from ../user/initcode.S by executing command:
// od -t xC user/initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Create the first proc, which runs the initcode above.
void userinit(void){
    // 1. alloc a unnused proc
    struct proc *p = allocproc();
    if (!p){
        panic("userinit");
    }
    initproc = p;

    // 2. allocate one user page and copy initcode's instructions and data into it.
    uvmfirst(p->pagetable, initcode, sizeof(initcode));
    p->sz = PGSIZE; //p->sz is the  size used memory of proc.

    // 3. prepare for the very first "sret" from kernel to user.
    p->trapframe->epc = 0;       // user program counter
    p->trapframe->sp  = PGSIZE;  // user stack pointer

    safestrcpy(p->name, "initcode", sizeof(p->name));
    // p->cwd = namei("/");

    p->state = RUNNABLE;

    // validate
   pte_t *pte = walk(p->pagetable, 0, 0);
   uchar* actualInitCode = (uchar*)PTE2PA(*pte);
   for (int i = 0; i < sizeof(initcode); i++){
       if (*actualInitCode != initcode[i]){
        panic("userinit");
       }
       actualInitCode++;
   }
}

// current proc yield and switch to scheduler.
// sechduler will find next runnable proc to context switch
void sched(){
    int intena;
    struct proc* p = current_proc();

    if(!holding(&p->lock))
        panic("sched p->lock");
    
    if (current_cpu()->noff != 1) // interupt turned off for only once
        panic("sched locks");

    if(p->state == RUNNING)
        panic("sched running");

    if(intr_get())
        panic("sched interruptible");

    intena = current_cpu()->intena;   // intr will be enabled in scheduler, so should restore after swtch return.
    swtch(&p->context, &current_cpu()->context);
    current_cpu()->intena = intena;
}

void yield(void){
    struct proc *p = current_proc();
    p->state = SLEEPING;
    sched();
}

void sleep(void* chan, struct spinlock *lk){
    struct proc *p = current_proc();

    acquire(&p->lock);
    release(lk);
    p->chan = chan;
    p->state = SLEEPING;
    sched();
    p->state = RUNNABLE;
    p->chan = 0;
    release(&p->lock);
    acquire(lk);
}

// Copy on write fork
pid_t fork(){
    struct proc *np, *p;
    p = current_proc();
    np = allocproc();

    // copy page table
    uvmcopy(p->pagetable, np->pagetable, p->sz);
    np->sz = p->sz;



    return 0;
}

// wake up all sleeping proc
void wakeup(void* chan){
    struct proc *p;
    for (p = proc; p < proc + NPROC; p++){
        acquire(&p->lock);
        if (p->chan == chan){
            p->state = RUNNABLE;
        }
        release(&p->lock);
    }
}