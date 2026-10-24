#include "copy.h"

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

static int parse_offset(const char *text, off_t *value)
{
    char *end = NULL;
    intmax_t number;

    errno = 0;
    number = strtoimax(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || number < 0 ||
        (intmax_t)(off_t)number != number) {
        return -1;
    }
    *value = (off_t)number;
    return 0;
}

int main(int argc, char *argv[])
{
    char buffer[64 * 1024];
    off_t offset;
    off_t blocksize;
    off_t remaining;
    int srcfd = -1;
    int destfd = -1;
    int result = EXIT_FAILURE;

    if (argc != 5 || parse_offset(argv[3], &offset) != 0 ||
        parse_offset(argv[4], &blocksize) != 0) {
        fprintf(stderr, "用法: %s 源文件 目标文件 偏移量 块大小\n", argv[0]);
        return EXIT_FAILURE;
    }

    srcfd = open(argv[1], O_RDONLY);
    if (srcfd == -1) {
        perror("copy: open 源文件");
        goto finish;
    }
    destfd = open(argv[2], O_WRONLY | O_CREAT, 0664);
    if (destfd == -1) {
        perror("copy: open 目标文件");
        goto finish;
    }
    if (lseek(srcfd, offset, SEEK_SET) == (off_t)-1 ||
        lseek(destfd, offset, SEEK_SET) == (off_t)-1) {
        perror("copy: lseek");
        goto finish;
    }

    printf("pid=%ld offset=%jd blocksize=%jd\n", (long)getpid(),
           (intmax_t)offset, (intmax_t)blocksize);
    fflush(stdout);

    remaining = blocksize;
    while (remaining > 0) {
        size_t amount = remaining < (off_t)sizeof(buffer)
                        ? (size_t)remaining : sizeof(buffer);
        ssize_t read_len = read(srcfd, buffer, amount);

        if (read_len < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("copy: read");
            goto finish;
        }
        if (read_len == 0) {
            break;
        }

        for (ssize_t written = 0; written < read_len;) {
            ssize_t write_len = write(destfd, buffer + written,
                                      (size_t)(read_len - written));
            if (write_len < 0 && errno == EINTR) {
                continue;
            }
            if (write_len <= 0) {
                if (write_len == 0) {
                    fprintf(stderr, "copy: write 未写入任何数据\n");
                } else {
                    perror("copy: write");
                }
                goto finish;
            }
            written += write_len;
        }
        remaining -= read_len;
    }
    result = EXIT_SUCCESS;

finish:
    if (srcfd != -1 && close(srcfd) == -1) {
        perror("copy: close 源文件");
        result = EXIT_FAILURE;
    }
    if (destfd != -1 && close(destfd) == -1) {
        perror("copy: close 目标文件");
        result = EXIT_FAILURE;
    }
    return result;
}
