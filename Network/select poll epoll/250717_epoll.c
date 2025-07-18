#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/epoll.h>


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

    // 5. 创建epoll模型
    int epfd = epoll_create(1);
    if (epfd == -1) {
        perror("epoll");
        exit(0);
    }

    // 6. 将要检测的节点添加到epoll模型中
    struct epoll_event ev;
    ev.events = EPOLLIN;  // 监听读事件
    ev.data.fd = sockfd;  // 将监听套接字添加到epoll模型
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
    if (ret == -1) {
        perror("epoll_ctl");
        return -1;
    }

    // 7. 不停地委托内核检测epoll模型中的文件描述符状态
    struct epoll_event events[1024];
    while (1) {
        int nready = epoll_wait(epfd, events, 1024, -1);
        for (int i = 0; i < nready; i++) {
            int curfd = events[i].data.fd;

            // 如果是监听套接字，说明有新连接
            if (curfd == sockfd) {

                struct sockaddr_in clientaddr;
                socklen_t len = sizeof(clientaddr);
                int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &len);

                ev.events = EPOLLIN;  
                ev.data.fd = clientfd;
                // 将客户端套接字添加到epoll模型
                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);

            } else {  //通信

                char buffer[1024] = {0};
                int count = recv(curfd, buffer, sizeof(buffer), 0);
                if (count == -1) {
                    perror("recv");
                    continue;
                } else if (count == 0) {
                    printf("Client disconnected\n");
                    close(curfd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, curfd, NULL);  // 从epoll模型中删除
                    continue;
                } 

                send(curfd, buffer, count, 0);
                printf("Received %d bytes: %s\n", count, buffer);
            }
        }

    }

    close(sockfd);
    
    return 0;
}