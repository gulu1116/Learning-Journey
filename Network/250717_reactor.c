#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_LENGTH 1024

struct conn_item {
    int fd;
    char buffer[BUFFER_LENGTH];
    int idx;  //
};

struct conn_item connlist[1024] = {0};

int epfd = 0;  // 全局变量，epoll文件描述符

// flag == 1  add
// flag == 0  modify
int set_event(int fd, int event, int flag) {

    if (flag) {
        struct epoll_event ev;
        ev.events = event;
        ev.data.fd = fd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    } else {
        struct epoll_event ev;
        ev.events = event;
        ev.data.fd = fd;
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    }
}



// listenfd
// 触发EPOLLIN
int accept_cb(int fd) {
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    int clientfd = accept(fd, (struct sockaddr *)&clientaddr, &len);
    if (clientfd < 0) {
        perror("select");
        return -1;
    }

    set_event(clientfd, EPOLLIN , 1);

    connlist[clientfd].fd = clientfd;  // 初始化连接项
    memset(connlist[clientfd].buffer, 0, BUFFER_LENGTH);
    connlist[clientfd].idx = 0;  // 初始化索引

    return clientfd;
}

// clientfd
int recv_cb(int fd) {
    char *buffer = connlist[fd].buffer;
    int idx = connlist[fd].idx;
    
    int count = recv(fd, buffer + idx, BUFFER_LENGTH - idx, 0);
    if (count == -1) {
        perror("recv");
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);  // 从epoll模型中删除
        return -1;

    } else if (count == 0) {
        printf("Client disconnected\n");
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);  // 从epoll模型中删除
        return 0;  // 客户端断开连接
    } 

    connlist[fd].idx += count;  // 更新索引
    printf("Received %d bytes: %s\n", count, buffer);

    // send(fd, buffer, idx, count);

    // 如果接收到了数据，修改事件为可写
    // 这里假设我们只在接收到数据后才允许发送数据
    set_event(fd, EPOLLOUT , 0);

    return count;  // 返回接收的字节数
}

int send_cb(int fd) {
    char *buffer = connlist[fd].buffer;
    int idx = connlist[fd].idx;
    int count = send(fd, buffer, idx, 0);
    if (count == -1) {
        perror("send");
        return -1;
    }

    printf("Sent %d bytes: %s\n", count,buffer);

    // 发送完数据后，修改事件为可读
    set_event(fd, EPOLLIN , 0);

    return count;
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

    // 5. 创建epoll模型
    epfd = epoll_create(1);
    if (epfd == -1) {
        perror("epoll");
        return -1;
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

                int clientfd = accept_cb(sockfd);

            } else {

                if (events[i].events & EPOLLIN) {  //通信
                    // 调用接收回调函数
                    int count = recv_cb(curfd);
                }

                if (events[i].events & EPOLLOUT) {
                    // 调用发送回调函数
                    int count = send_cb(curfd);
                }
            }
        }
    }

    close(sockfd);
    
    return 0;
}