#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "file.h"
#include "buf.h"
#include "net.h"
#include "defs.h"
#include "memlayout.h"

#define PIPESIZE 512

struct pipe {
    struct spinlock lock;

    // circular array
    char data[PIPESIZE];
    int ridx;
    int widx;
    int readopen;
    int writeopen;
};

int
pipealloc(struct file **rf, struct file **wf){
    struct pipe *p;
    *rf = 0, *wf = 0;
    if ((*rf = filealloc()) == 0 || (*wf = filealloc()) == 0){
        goto failure;
    }

    if ((p = (struct pipe*)kalloc()) == 0){
        goto failure;
    }

    (*rf)->type = FD_PIPE;
    (*rf)->pipe = p;
    (*rf)->readable = 1;
    (*rf)->writable = 0;

    (*wf)->type = FD_PIPE;
    (*wf)->pipe = p;
    (*wf)->readable = 0;
    (*wf)->writable = 1;   
    return 0;

failure:
    if (*rf){
        fileclose(*rf);
    }

    if (*wf){
        fileclose(*wf);
    }

    if (p){
        kfree(p);
    }
    return -1;
}

void
pipeclose(struct pipe *p, int writable){
    acquire(&p->lock);
    if(writable){
        p->writeopen = 0;
        wakeup(&p->ridx);
    } else {
        p->readopen = 0;
        wakeup(&p->widx);
    }
    if(p->readopen == 0 && p->writeopen == 0){
        release(&p->lock);
        kfree((char*)p);
    } else{
        release(&p->lock);
    }
}

int
piperead(struct pipe *p, uint64 usr_dst, int len){
    struct proc *proc;
    int i;
    char ch;

    acquire(&p->lock);
    while(p->ridx == p->widx && p->writeopen){
        sleep(&p->ridx, &p->lock);
    }

    proc = current_proc();
    for (i = 0; p->ridx != p->widx && i < len; i++){
        ch = p->data[p->ridx++ % PIPESIZE];
        if(copyout(proc->pagetable, usr_dst + i, &ch, 1) == -1){
            break;
        }
    }
    wakeup(&p->widx);  //DOC: piperead-wakeup
    release(&p->lock);
    return i;
}

int
pipewrite(struct pipe *p, uint64 usr_src, int n){
    int i = 0;
    struct proc *pr = current_proc();

    acquire(&p->lock);
    while(i < n){
        if(p->readopen == 0){
            release(&p->lock);
            return -1;
        }
        if(p->widx == p->ridx + PIPESIZE){ //DOC: pipewrite-full
            wakeup(&p->ridx);
            sleep(&p->widx, &p->lock);
        } 
        else {
            char ch;
            if(copyin(pr->pagetable, &ch, usr_src + i, 1) == -1)
                break;
            p->data[p->widx++ % PIPESIZE] = ch;
            i++;
        }
    }
    wakeup(&p->ridx);
    release(&p->lock);

    return i;
}
