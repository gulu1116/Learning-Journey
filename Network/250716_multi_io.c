#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

void *client_thread(void *arg) {

    int clientfd = *(int *)arg;

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

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(struct sockaddr_in));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(8888);  // 端口号
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);  // 绑定到所有可用的接口

    int ret = bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (ret == -1) {
        perror("bind");
        return -1;
    }

    ret = listen(sockfd, 10);
    if (ret == -1) {
        perror("listen");
        return -1;
    }

    while (1) {

        struct sockaddr_in clientaddr;
        socklen_t len = sizeof(clientaddr);
        int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &len);
        if (clientfd == -1) {
            perror("accept");
            return -1;
        }

        pthread_t thid;
        pthread_create(&thid, NULL, client_thread, &clientfd);

    }

    return 0;

}