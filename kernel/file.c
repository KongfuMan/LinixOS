#include "param.h"
#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "defs.h"
#include "file.h"

struct file* filealloc(){
    return 0;
}

void fileclose(struct file* file){

}

struct file* filedup(struct file* file){
    return 0;
}

void fileinit(void){

}

// write `n` bytes from file f to user virtual address `addr`
// return actual number of bytes read.
int fileread(struct file* file, uint64 addr, int n){
    // 1. copy from user space to kernel space
    // 2. write to file.
    return 0;
}

int filestat(struct file* file, uint64 addr){
    return 0;
}

// write `n` bytes from file f to user virtual address `addr`
// return actual number of bytes written.
int filewrite(struct file* file, uint64 addr, int n){
    return 0;
}