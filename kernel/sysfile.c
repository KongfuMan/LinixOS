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

// 1. load the binary (with elf format) into memory from disk by the file path
// 2. create a chid process to run the binary
uint64 
sys_exec(void){
    // arguments: a0, a1
    // - a0: path of binary image
    // - a1: arguments for binary
    char path[MAXPATH], *argv[MAXARG];
    int i;
    uint64 uargv, uarg;

    argaddr(1, &uargv);
    if(argstr(0, path, MAXPATH) < 0) {
        return -1;
    }

    memset(argv, 0, sizeof(argv)); 

    for(i=0;; i++){
        if(i >= NELEM(argv)){
            goto bad;
        }
        if(fetchaddr(uargv+sizeof(uint64)*i, (uint64*)&uarg) < 0){
            goto bad;
        }
        if(uarg == 0){
            argv[i] = 0;
            break;
        }
        argv[i] = kalloc();
        if(argv[i] == 0){
            goto bad;
        }
        if(fetchstr(uarg, argv[i], PGSIZE) < 0){
            goto bad;
        }
    }

    int ret = exec(path, argv);

    for(i = 0; i < NELEM(argv) && argv[i] != 0; i++)
        kfree(argv[i]);

    return ret;

    bad:
        for(i = 0; i < NELEM(argv) && argv[i] != 0; i++)
            kfree(argv[i]);
        return -1;
}