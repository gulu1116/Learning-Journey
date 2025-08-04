#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MSG_TRY "try again\n"

int main(void) {
    char buf[10];
    int flags, n;

    flags = fcntl(STDIN_FILENO, F_GETFL);  // 获取 stdin 属性信息
    if (flags == -1) {
        perror("fcntl error");
        exit(1);
    }

    flags |= O_NONBLOCK;
    int ret = fcntl(STDIN_FILENO, F_SETFL, flags);
    if (ret == -1) {
        perror("fcntl error");
        exit(1);
    }

tryagain:
    n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n < 0) {
        if (errno == EAGAIN){
            printf("没有数据可读（非阻塞返回）\n");
            write(STDOUT_FILENO, "try again\n", strlen("try again\n"));
            sleep(2);
            goto tryagain;
        } else {
            perror("read /dev/tty");
            return -1;
        }
    } else {
        write(STDOUT_FILENO, buf, n);
    }

    return 0;
}