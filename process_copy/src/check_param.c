#include "copy.h"

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

int check_param(int argc, const char *srcfile, int pronum)
{
    struct stat info;

    if ((argc != 3 && argc != 4) || pronum < 1 || pronum > 100) {
        fprintf(stderr, "参数不合法\n");
        return -1;
    }
    if (access(srcfile, F_OK) != 0) {
        perror("源文件不存在");
        return -1;
    }
    if (stat(srcfile, &info) == -1 || !S_ISREG(info.st_mode)) {
        fprintf(stderr, "源文件必须是可读取的普通文件\n");
        return -1;
    }
    if (access(srcfile, R_OK) != 0) {
        perror("源文件不可读");
        return -1;
    }
    return 0;
}
