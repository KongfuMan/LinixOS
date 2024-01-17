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
  int n;
  argint(0, &n);
  exit(n);
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

uint64
sys_sigalarm(void){
  int total;
  uint64 handler;

  argint(0, &total);
  if (total < 0){
    return -1;
  }

  argaddr(1, &handler);
  if (handler == 0){
    return -1;
  }
  
  struct proc *p = current_proc();
  acquire(&p->lock);
  p->total = total;
  p->handler = handler;
  release(&p->lock);
  return 0;
}

uint64
sys_sigreturn(void){
  struct proc *p = current_proc();
  acquire(&p->lock);
  *p->trapframe = *p->alarmframe;
  p->alarmstate = 0;
  release(&p->lock);
  return p->trapframe->a0;
}