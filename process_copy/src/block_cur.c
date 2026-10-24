#include "copy.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

off_t block_cur(const char *srcfile, int pronum)
{
    int fd;
    off_t filesize;

    if (pronum <= 0) {
        fprintf(stderr, "进程数必须大于 0\n");
        return -1;
    }
    fd = open(srcfile, O_RDONLY);
    if (fd == -1) {
        perror("open 源文件");
        return -1;
    }
    filesize = lseek(fd, 0, SEEK_END);
    if (filesize == (off_t)-1) {
        perror("lseek 文件末尾");
    }
    if (close(fd) == -1) {
        perror("close 源文件");
        return -1;
    }
    if (filesize == (off_t)-1) {
        return -1;
    }
    return filesize / pronum + (filesize % pronum != 0);
}
