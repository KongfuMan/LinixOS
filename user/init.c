#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// spawn a shell process
int main(){
    if ((open("console", O_RDWR)) < 0){
        mknod("console", CONSOLE, 0);
        open("console", O_RDWR);
    }
    
    while(1){
        // command = read(fd, buf, len);
    }
    return 0;
}