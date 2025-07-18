#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

#define ENABLE_HTTP_RESPONSE 1


#define BUFFER_LENGTH 1024

typedef int (*event_callback)(int fd);

// listenfd ---> accept_cb
int accept_cb(int fd);
// clientfd ---> recv_cb, send_cb
int recv_cb(int fd);
int send_cb(int fd);

struct conn_item {
    int fd;

    char rbuffer[BUFFER_LENGTH];
    int rlen;
    char wbuffer[BUFFER_LENGTH];
    int wlen;

    union {
        event_callback accept_callback;  // 接收回调函数
        event_callback recv_callback;    // 接收回调函数
    } recv_type;
    event_callback send_callback;    // 发送回调函数
};

struct conn_item connlist[1024] = {0};

int epfd = 0;  // 全局变量，epoll文件描述符


#if ENABLE_HTTP_RESPONSE

typedef struct conn_item connection_t;

int http_request(connection_t *c) {

	//printf("request: %s\n", c->rbuffer);

	memset(c->wbuffer, 0, BUFFER_LENGTH);
	c->wlen = 0;

}

int http_response(connection_t *conn) {

#if 1
    conn->wlen = sprintf(conn->wbuffer, 
        "HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Accept-Ranges: bytes\r\n"
		"Content-Length: 82\r\n"
		"Date: Tue, 30 Apr 2024 13:16:46 GMT\r\n\r\n"
		"<html><head><title>Gdpu.GuLu</title></head><body><h1>Hello, GuLu</h1></body></html>\r\n\r\n");
#else

#endif

    return conn->wlen;
}

#endif 


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
    memset(connlist[clientfd].rbuffer, 0, BUFFER_LENGTH);
    memset(connlist[clientfd].wbuffer, 0, BUFFER_LENGTH);
    connlist[clientfd].rlen = 0;
    connlist[clientfd].wlen = 0;
    connlist[clientfd].recv_type.recv_callback = recv_cb;      // 设置接收回
    connlist[clientfd].send_callback = send_cb;      // 设置发送回调函数

    return clientfd;
}

// clientfd
int recv_cb(int fd) {
    char *buffer = connlist[fd].rbuffer;
    int idx = connlist[fd].rlen;
    
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

    connlist[fd].rlen += count;  // 更新索引
    printf("Received %d bytes: %s\n", count, buffer);

#if 0
    memcpy(connlist[fd].wbuffer, buffer, connlist[fd].rlen);  // 将接收到的数据复制到发送缓冲区
    connlist[fd].wlen = connlist[fd].rlen;  //
#else
    // http_requset(&connlist[fd]);
    http_response(&connlist[fd]);
#endif

    // 如果接收到了数据，修改事件为可写
    set_event(fd, EPOLLOUT , 0);

    return count;  // 返回接收的字节数
}

int send_cb(int fd) {
    char *buffer = connlist[fd].wbuffer;
    int idx = connlist[fd].wlen;
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

    connlist[sockfd].fd = sockfd;  // 初始化监听套接字连接项
    connlist[sockfd].recv_type.accept_callback = accept_cb;  // 设置接收回调函数

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
    set_event(sockfd, EPOLLIN, 1);  // 监听套接字，触发EPOLLIN事件

    // 7. 不停地委托内核检测epoll模型中的文件描述符状态
    struct epoll_event events[1024];
    while (1) {
        int nready = epoll_wait(epfd, events, 1024, -1);
        for (int i = 0; i < nready; i++) {
            int curfd = events[i].data.fd;

            if (events[i].events & EPOLLIN) {

                int count = connlist[curfd].recv_type.recv_callback(curfd);
            }

            if (events[i].events & EPOLLOUT) {
                
                int count = connlist[curfd].send_callback(curfd);
            }
        }
    }

    close(sockfd);
    
    return 0;
}