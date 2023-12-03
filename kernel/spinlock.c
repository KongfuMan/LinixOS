
#include "types.h"
#include "param.h"
#include "riscv.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "defs.h"


void initlock(struct spinlock *lk, char *name){
    lk->locked = 0;
    lk->name = name;
    lk->cpu = 0;
}

void acquire(struct spinlock *lk){
    push_off(); // disable intr of current hart 
    if (holding(lk)){
        panic("acquire: already held the lock");
    }

    while(__sync_lock_test_and_set(&(lk->locked), 1) == 1){}

    __sync_synchronize();
    lk->cpu = current_cpu();
}

void release(struct spinlock *lk){
    if (!holding(lk)){
        panic("release: not hold lock");
    }

    lk->cpu = 0;

    __sync_synchronize();

    // Release the lock, equivalent to lk->locked = 0.
    // This code doesn't use a C assignment, since the C standard
    // implies that an assignment might be implemented with
    // multiple store instructions.
    // On RISC-V, sync_lock_release turns into an atomic swap:
    //   s1 = &lk->locked
    //   amoswap.w zero, zero, (s1)
    __sync_lock_release(&lk->locked);
     
    pop_off();
}

void push_off(){
    int old = intr_get(); //get current interrupt status
    intr_off();
    struct cpu *curr_cpu = current_cpu();
    if (curr_cpu->noff == 0){
        curr_cpu->intena = old;
    }
    
    curr_cpu->noff += 1;
}

void pop_off(){
    if (intr_get()){
        panic("pop_off - interruptible");
    }

    struct cpu *curr_cpu = current_cpu();

    if (curr_cpu->noff < 1){
        panic("pop_off");
    }

    curr_cpu->noff -= 1;
    if (curr_cpu->noff == 0 && curr_cpu->intena){
        intr_on();
    }
}

int holding(struct spinlock *lk){
    if (lk->locked && lk->cpu == current_cpu()){
        return 1;
    }
    return 0;
}