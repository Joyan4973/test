#include "copy.h"

#include <inttypes.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int process_create(const char *srcfile, const char *destfile,
                   int pronum, off_t blocksize, const char *copy_path)
{
    int created = 0;

    for (int i = 0; i < pronum; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            break;
        }
        if (pid == 0) {
            char str_offset[32];
            char str_blocksize[32];
            off_t offset = (off_t)i * blocksize;

            if (snprintf(str_offset, sizeof(str_offset), "%jd", (intmax_t)offset)
                    >= (int)sizeof(str_offset) ||
                snprintf(str_blocksize, sizeof(str_blocksize), "%jd",
                         (intmax_t)blocksize) >= (int)sizeof(str_blocksize)) {
                fprintf(stderr, "偏移量或块大小超出参数缓冲区\n");
                _exit(127);
            }
            execl(copy_path, "copy", srcfile, destfile,
                  str_offset, str_blocksize, (char *)NULL);
            perror("execl copy");
            _exit(127);
        }
        ++created;
    }
    return created;
}
