#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <poll.h>


int main() {
    // 1. 创建 socket（监听套接字）
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // 2. 配置服务器地址结构
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(struct sockaddr_in));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(8888);  // 端口号
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);  // 绑定到所有可用的接口

    // 3. 绑定地址和端口
    int ret = bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (ret == -1) {
        perror("bind");
        return -1;
    }

    // 4. 开始监听（最大等待队列长度为10）
    ret = listen(sockfd, 10);
    if (ret == -1) {
        perror("listen");
        return -1;
    }

    // 5. 等待连接 -> 循环
    // 检测 -> 读缓冲区，委托内核去处理
    // 数据初始化，创建自定义的文件描述符
    struct pollfd fds[1024];
    for (int i = 0; i < 1024; i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }

    fds[0].fd = sockfd;  // 将监听套接字添加到 pollfd 数组

    int maxfd = sockfd;

    while (1) {

        int nready = poll(fds, maxfd+1, -1);
        if (nready == -1) {
            perror("select");
            return -1;
        }

        // 检测的读缓冲区有变化
        // 有新连接
        if (fds[0].revents & POLLIN) {

            // 接收连接请求
            struct sockaddr_in clientaddr;
            socklen_t addrlen = sizeof(clientaddr);
            int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &addrlen);
            
            fds[clientfd].fd = clientfd;  // 将新连接添加到 pollfd 数组
            maxfd = clientfd;
        }

        // 通信，有客户端发送数据过来
        for (int i = sockfd + 1; i <= maxfd; i++) {
            if (fds[i].revents & POLLIN) {
                char buffer[128] = {0};
                int count = recv(i, buffer, sizeof(buffer), 0);
                if (count == -1) {
                    perror("recv");
                    close(i);
                    fds[i].fd = -1;  // 将该文件描述符标记为无效
                } else if (count == 0) {
                    printf("Client disconnected\n");
                    close(i);
                    fds[i].fd = -1;  // 将该文件描述符标记为无效
                    break;
                } else {
                    send(i, buffer, count, 0);
                    printf("Received %d bytes: %s\n", count, buffer);
                }
            }
        }
    }

    close(sockfd);
    
    return 0;
}