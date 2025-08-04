#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

// 客户端处理线程函数：收发数据
void *client_thread(void *arg) {

    int clientfd = *(int *)arg;  // 获取客户端连接的 socket

    while (1) {

        char buffer[1024] = {0};
        int count = recv(clientfd, buffer, sizeof(buffer), 0);
        if (count == -1) {
            perror("recv");
            // return -1;
        } else if (count == 0) {
            printf("Client disconnected\n");
            break;
        } 
        
        send(clientfd, buffer, count, 0);
        printf("Received %d bytes: %s\n", count, buffer);
    }

    close(clientfd);
}

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

    // 5. 循环接收客户端连接
    while (1) {

        struct sockaddr_in clientaddr;
        socklen_t len = sizeof(clientaddr);
        int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &len);
        if (clientfd == -1) {
            perror("accept");
            return -1;
        }

        // 6. 创建线程处理客户端连接
        pthread_t thid;
        pthread_create(&thid, NULL, client_thread, &clientfd);
    }

    return 0;
}