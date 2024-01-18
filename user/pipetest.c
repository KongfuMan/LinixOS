#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int
main(){
    int pipefd[2];
    pid_t cpid;
    char buf;

    if (pipe(pipefd) == -1) {
        exit(-1);
    }
    cpid = fork();
    if (cpid == -1) {
        exit(-1);
    }

    if (cpid == 0) {    /* Child reads from pipe */
        close(pipefd[1]);          /* Close unused write end */

        while (read(pipefd[0], &buf, 1) > 0)
            write(1, &buf, 1);

        write(1, "\n", 1);
        close(pipefd[0]);
        exit(0);

    } else {            /* Parent writes argv[1] to pipe */
        close(pipefd[0]);          /* Close unused read end */
        // write(pipefd[1], argv[1], strlen(argv[1]));
        close(pipefd[1]);          /* Reader will see EOF */
        wait(0);                /* Wait for child */
        exit(0);
    }
}