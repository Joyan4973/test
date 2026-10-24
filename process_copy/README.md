# 多进程文件拷贝

`src/process_copy` 分配文件区间并启动子进程，`mod/copy` 拷贝对应的数据。进程数可省略，默认是 1，最多 100。

## 编译

在 Ubuntu 终端执行：

```bash
cd process_copy/mod
gcc -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror copy.c -I../include -o copy
cd ../src
gcc -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror *.c -I../include -o process_copy
```

## 运行

在 `process_copy` 目录执行：

```bash
./src/process_copy 1.png A.png 5
cmp 1.png A.png
```

进程数不写时默认用 1 个进程。`cmp` 没有输出，表示两个文件内容相同。

## 测试

```bash
bash tests/test_copy.sh
```
