#define _POSIX_C_SOURCE 200809L

#include "server.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

struct client {
    int sock;
    size_t len;
    int too_long;
    char line[BUFFER_SIZE];
};

static int net_initializer(const char *ip, int port)
{
    int server_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    struct sockaddr_in addr = {0};
    int reuse = 1;

    if (server_sock < 0) {
        perror("socket");
        return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "invalid IPv4 address: %s\n", ip);
    } else if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR,
                          &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
    } else if (bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
    } else if (listen(server_sock, BACKLOG) < 0) {
        perror("listen");
    } else {
        return server_sock;
    }
    close(server_sock);
    return -1;
}

static int send_text(int sock, const char *text)
{
    size_t len = strlen(text);

    while (len > 0) {
        ssize_t n = send(sock, text, len, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (n < 0 && errno == EINTR)
            continue;
        /* 只监听读集合；发送缓冲区满时关闭该连接，不阻塞主循环。 */
        if (n <= 0)
            return -1;
        text += n;
        len -= (size_t)n;
    }
    return 0;
}

static int first_response(int sock, const char *ip)
{
    char response[80];
    snprintf(response, sizeof(response), "Hi ,[%s] connect server success.\n", ip);
    return send_text(sock, response);
}

static int reply(struct client *client)
{
    size_t len = client->len;
    if (len > 0 && client->line[len - 1] == '\r')
        --len;

    if (!client->too_long && len == 4 && memcmp(client->line, "time", 4) == 0) {
        time_t now = time(NULL);
        char time_buf[32];
        if (now != (time_t)-1 && ctime_r(&now, time_buf) != NULL)
            return send_text(client->sock, time_buf);
    }
    return send_text(client->sock, "Please tryagain.\n");
}

static int handle_client(struct client *client)
{
    char response[BUFFER_SIZE];
    ssize_t n = recv(client->sock, response, sizeof(response), MSG_DONTWAIT);

    if (n < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
        return 0;
    if (n <= 0) {
        if (n < 0)
            perror("recv");
        else if (client->len > 0 || client->too_long)
            (void)reply(client);
        return -1;
    }

    /* TCP 按字节收发：一行可能分几次收到，一次也可能收到多行。 */
    for (ssize_t i = 0; i < n; ++i) {
        if (response[i] == '\n') {
            if (reply(client) < 0)
                return -1;
            client->len = 0;
            client->too_long = 0;
        } else if (client->len < sizeof(client->line)) {
            client->line[client->len++] = response[i];
        } else {
            client->too_long = 1;
        }
    }
    return 0;
}

static void accept_client(int server_sock, struct client *clients,
                          fd_set *set, int *maxfd)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    char ip[INET_ADDRSTRLEN];
    int client_sock = accept(server_sock, (struct sockaddr *)&addr, &addrlen);

    if (client_sock < 0) {
        if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
            perror("accept");
        return;
    }
    int i = 0;
    while (i < MAX_CLIENTS && clients[i].sock != -1)
        ++i;
    if (i == MAX_CLIENTS || client_sock >= FD_SETSIZE) {
        close(client_sock);
        return;
    }

    const char *cip = inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    if (cip == NULL || first_response(client_sock, cip) < 0) {
        close(client_sock);
        return;
    }
    clients[i].sock = client_sock;
    clients[i].len = 0;
    clients[i].too_long = 0;
    FD_SET(client_sock, set);
    if (client_sock > *maxfd)
        *maxfd = client_sock;

    printf("connect success, cip %s cport %u\n", cip, (unsigned)ntohs(addr.sin_port));
}

static int server_start(void)
{
    int server_sock = net_initializer(SERVER_IP, SERVER_PORT);
    if (server_sock < 0)
        return 1;
    if (server_sock >= FD_SETSIZE) {
        fprintf(stderr, "server socket exceeds FD_SETSIZE\n");
        close(server_sock);
        return 1;
    }

    static struct client client_io_array[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; ++i)
        client_io_array[i].sock = -1;

    fd_set set, oset;
    FD_ZERO(&set);
    FD_SET(server_sock, &set);
    int maxfd = server_sock;
    printf("select server (%s:%d) listening...\n", SERVER_IP, SERVER_PORT);
    fflush(stdout);

    for (;;) {
        /* set 保存监听状态，oset 只存这一轮的就绪结果。 */
        oset = set;
        int readynum = select(maxfd + 1, &oset, NULL, NULL, NULL);
        if (readynum < 0) {
            if (errno == EINTR)
                continue;
            perror("select");
            break;
        }

        if (FD_ISSET(server_sock, &oset)) {
            accept_client(server_sock, client_io_array, &set, &maxfd);
            --readynum;
        }
        for (int i = 0; i < MAX_CLIENTS && readynum > 0; ++i) {
            int sock = client_io_array[i].sock;
            if (sock < 0 || !FD_ISSET(sock, &oset))
                continue;
            --readynum;
            if (handle_client(&client_io_array[i]) < 0) {
                FD_CLR(sock, &set);
                close(sock);
                client_io_array[i].sock = -1;
                while (maxfd > server_sock && !FD_ISSET(maxfd, &set))
                    --maxfd;
                printf("client exit, close client_sock = %d\n", sock);
            }
        }
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_io_array[i].sock >= 0)
            close(client_io_array[i].sock);
    }
    close(server_sock);
    return 1;
}

int main(void)
{
    return server_start();
}
