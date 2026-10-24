# 多进程拷贝作业

工程按课堂的两个程序组织：`src/process_copy` 负责校验、切片、创建和回收子进程；`mod/copy` 由各子进程通过 `execl` 加载，负责拷贝分配到的文件区间。进程数可省略，默认 1；允许 1～100。

## 在 Ubuntu 编译

```bash
cd process_copy/mod
gcc -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror copy.c -I../include -o copy
cd ../src
gcc -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror *.c -I../include -o process_copy
```

两个目录各有一个 `main`，必须分别编译，不能把 `mod/copy.c` 与 `src/*.c` 链接成同一个程序。主程序从自身所在目录定位 `../mod/copy`，所以可在任意工作目录调用它。

## 使用

```bash
./src/process_copy 1.png A.png 5
./src/process_copy 1.png A.png
cmp 1.png A.png
```

`cmp` 没有输出且退出码为 0，表示两文件逐字节一致。调试时每个 `copy` 子进程会打印 PID、起始偏移量和分配的块大小；这些数字并不表示实际读到的字节数，末尾进程可能提前遇到 EOF。

## 实现说明

- 分块大小为 `文件大小 / 进程数`，不能整除时加 1；第 `i` 个进程从 `i * blocksize` 开始。
- 父进程只创建并截断一次目标文件，子进程用各自的文件描述符在不重叠的区间读写。源与目标为同一文件时拒绝拷贝。
- `copy` 使用固定 64 KiB 缓冲区循环拷贝，按 `read` 返回的长度写入，处理短写及被信号中断的系统调用；不把二进制文件当字符串处理。
- 文件大小与偏移量使用 `off_t`，传给 `execl` 前转成字符串。父进程检查每个子进程的退出状态。

可运行 `bash tests/test_copy.sh` 完成编译和基本回归测试。课堂课件只给出接口示意，以上错误处理和边界情况是在相同流程上补齐的作业实现，不是老师课堂逐行代码。
