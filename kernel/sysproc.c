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
#include "stat.h"

uint64
sys_exit(){
    panic("sys_exit: not impplemented.");
    return -1;
}