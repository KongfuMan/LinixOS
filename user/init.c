#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include "kernel/file.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char *argv[] = { "sh", 0 };

// spawn a shell process
int main(){
    int pid, wid;
    int sz;
    if (open("console", O_RDWR) < 0){
        mknod("console", CONSOLE, 0);
        open("console", O_RDWR);
    }

    dup(0); // fd=1 for stdout
    dup(0); // fd=2 for stderr

    for(;;){
        // fork a proc to run shell.
        pid = fork();
        if (pid < 0){
            exit(1);
        }
        if (pid == 0){
            exec("sh", argv); // child proc: replace bin image with `sh`
            exit(1); // shell proc should never return unless run into exception.
        }

        for (;;){
            wid = wait((int *) 0); // init proc keep waiting for child proc to terminate
            if (wid == pid){
                // start a new one when child shell proc terminated
                break;
            }else if (wid < 0){
                // wait return error
                exit(1);
            }else {
                // it was a childless proc; do nothing.
            }
        }
    }
}