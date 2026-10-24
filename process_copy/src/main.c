#include "copy.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int pronum = 1;
    struct stat src_stat;
    struct stat dest_stat;
    char exe_path[PATH_MAX];
    char copy_path[PATH_MAX];
    ssize_t path_len;
    char *slash;
    off_t blocksize;
    int destfd;
    int child_count;
    int wait_result;

    if (argc != 3 && argc != 4) {
        fprintf(stderr, "用法: %s 源文件 目标文件 [进程数]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 4) {
        char *end = NULL;
        long value;

        errno = 0;
        value = strtol(argv[3], &end, 10);
        if (errno != 0 || end == argv[3] || *end != '\0' ||
            value < 1 || value > 100) {
            fprintf(stderr, "进程数必须是 1 到 100 的整数\n");
            return EXIT_FAILURE;
        }
        pronum = (int)value;
    }

    if (check_param(argc, argv[1], pronum) != 0) {
        return EXIT_FAILURE;
    }

    if (stat(argv[1], &src_stat) == -1) {
        perror("stat 源文件");
        return EXIT_FAILURE;
    }
    if (stat(argv[2], &dest_stat) == 0) {
        if (!S_ISREG(dest_stat.st_mode)) {
            fprintf(stderr, "目标必须是普通文件\n");
            return EXIT_FAILURE;
        }
        if (src_stat.st_dev == dest_stat.st_dev &&
            src_stat.st_ino == dest_stat.st_ino) {
            fprintf(stderr, "源文件和目标文件不能是同一个文件\n");
            return EXIT_FAILURE;
        }
    } else if (errno != ENOENT) {
        perror("stat 目标文件");
        return EXIT_FAILURE;
    }

    blocksize = block_cur(argv[1], pronum);
    if (blocksize < 0) {
        return EXIT_FAILURE;
    }

    path_len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (path_len < 0 || (size_t)path_len >= sizeof(exe_path) - 1) {
        perror("确定主程序路径");
        return EXIT_FAILURE;
    }
    exe_path[path_len] = '\0';
    slash = strrchr(exe_path, '/');
    if (slash == NULL) {
        fprintf(stderr, "无法确定主程序目录\n");
        return EXIT_FAILURE;
    }
    *slash = '\0';
    if (snprintf(copy_path, sizeof(copy_path), "%s/../mod/copy", exe_path)
        >= (int)sizeof(copy_path)) {
        fprintf(stderr, "Copy 程序路径过长\n");
        return EXIT_FAILURE;
    }
    if (access(copy_path, X_OK) != 0) {
        perror("Copy 程序不可执行，请先编译 mod/copy.c");
        return EXIT_FAILURE;
    }

    /* 只由父进程创建并截断一次，子进程分别写入自己的区间。 */
    destfd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (destfd == -1) {
        perror("open 目标文件");
        return EXIT_FAILURE;
    }
    if (ftruncate(destfd, src_stat.st_size) == -1) {
        perror("ftruncate 目标文件");
        close(destfd);
        return EXIT_FAILURE;
    }
    if (close(destfd) == -1) {
        perror("close 目标文件");
        return EXIT_FAILURE;
    }

    if (src_stat.st_size == 0) {
        return EXIT_SUCCESS;
    }

    child_count = process_create(argv[1], argv[2], pronum, blocksize,
                                 copy_path);
    wait_result = process_wait(child_count);
    if (child_count != pronum || wait_result != 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
