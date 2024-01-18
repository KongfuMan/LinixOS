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

#define PIPENUM 512

struct pipe {
    struct spinlock lock;

    // circular array
    char data[PIPENUM];
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

int
pipeclose(struct pipe *p){

    return -1;
}

int
piperead(struct pipe *p, uint64 usr_dst, int len){
    return -1;
}

int
pipewrite(struct pipe *p, uint64 usr_src, int len){
    return -1;
}
