#ifndef COPY_H
#define COPY_H

#include <sys/types.h>

int check_param(int argc, const char *srcfile, int pronum);
off_t block_cur(const char *srcfile, int pronum);
int process_create(const char *srcfile, const char *destfile,
                   int pronum, off_t blocksize, const char *copy_path);
int process_wait(int child_count);

#endif
