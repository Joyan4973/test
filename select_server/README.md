# select 单进程时间服务器

使用一个进程接入 TCP 客户端并处理请求。长期关注集合是 `set`，每轮复制到 `oset` 后交给 `select`。监听套接字就绪时接入新客户端，客户端套接字就绪时读取数据。断开后从数组和监听集合移除该套接字。

## 编译运行

在 Ubuntu 中进入本目录：

```bash
gcc -std=c11 -Wall -Wextra select_server.c -o server
./server
```

在同一台 Ubuntu 中另开终端：

```bash
nc -v 127.0.0.1 8080
```

连接后会收到欢迎消息。输入 `time` 并回车，返回服务器本地时间；其他命令返回 `Please tryagain.`。可以开两个客户端分别验证。要从另一台机器连接，先将 `server.h` 中的 `SERVER_IP` 改为服务器网卡上的 IPv4 地址，再重新编译。

## 自测

```bash
python3 test_select_server.py
```

测试使用临时端口和二进制文件，验证两个客户端交替读写、分段收包、一次到达多条命令，以及断开后重新接入。
