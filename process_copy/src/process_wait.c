#include "copy.h"

#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>

int process_wait(int child_count)
{
    int result = 0;

    for (int i = 0; i < child_count;) {
        int status;
        pid_t pid = waitpid(-1, &status, 0);

        if (pid < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("waitpid");
            return -1;
        }
        ++i;
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "子进程 %ld 拷贝失败\n", (long)pid);
            result = -1;
        }
    }
    return result;
}
