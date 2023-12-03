#include "types.h"
#include "param.h"
#include "riscv.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "defs.h"

void
initsleeplock(struct sleeplock *lk, char *name)
{
    initlock(&lk->lk, "sleep lock");
    lk->name = name;
    lk->locked = 0;
    lk->pid = 0;
}

void acquiresleep(struct sleeplock *sleeplk){
    acquire(&sleeplk->lk);

    while(sleeplk->locked){
        sleep(sleeplk, &sleeplk->lk); // yield and switch to next runnable proc.
    }

    sleeplk->locked = 1;
    sleeplk->pid = current_proc()->pid;
    release(&sleeplk->lk);
}

void releasesleep(struct sleeplock *sleeplk){
    acquire(&sleeplk->lk);
    sleeplk->locked = 0;
    sleeplk->pid = 0;
    wakeup(sleeplk);
    release(&sleeplk->lk);
}

// Return if a sleep lock is being held by current process
int holdingsleep(struct sleeplock *sleeplk){
    int ret = 0;
    acquire(&sleeplk->lk);
    ret = sleeplk->locked && sleeplk->pid == current_proc()->pid;
    release(&sleeplk->lk);
    return ret;
}