#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
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

uint64
pipealloc(struct file **rf, struct file **wf){
    return -1;
}

uint64
pipeclose(struct pipe *p){
    return -1;
}

uint64
piperead(struct pipe *p, uint64 usr_dst, int len){
    return -1;
}

uint64
pipewrite(struct pipe *p, uint64 usr_src, int len){
    return -1;
}
