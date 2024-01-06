#include "param.h"
#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "net.h"
#include "defs.h"
#include "file.h"
#include "stat.h"

uint64
sys_exit(void){
  // panic("sys_exit: not impplemented.");
  return 0;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void){
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void){
  uint64 addr;
  int n;

  argint(0, &n);
  addr = current_proc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}