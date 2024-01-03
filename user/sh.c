#include "kernel/types.h"
#include "kernel/param.h"
#include "user.h"

int main(){
    int fd = 0;
    char args[MAXARG];
    int n;
    while(1){
        if ((n = read(fd, (void *)args, MAXARG)) < 0){
            // exit(1);
        }
    }

    return 0;
}