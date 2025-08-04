#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define MSG_TRY "try again\n"
#define MSG_TIMEOUT "time out\n"

int main(void) 
{
    // /dev/tty --> 当前终端设备
    // 以不阻塞方式(O_NONBLOCK)打开终端设备
    int fd = open("/dev/tty", O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open /dev/tty");
        return -1;
    }
    printf("open fd: %d\n", fd);

    char buf[10];
    int i = 0, n = 0;
    for (i = 0; i < 5; i++) {
        n = read(fd, buf, sizeof(buf));

        if (n < 0) {
            if (errno == EAGAIN){
                printf("没有数据可读（非阻塞返回）\n");
                write(STDOUT_FILENO, MSG_TRY, strlen(MSG_TRY));
                sleep(2);
            } else {
                perror("read /dev/tty");
                return -1;
            }
        } 
    }
    
    if (i == 5) {
        write(STDOUT_FILENO, MSG_TIMEOUT, strlen(MSG_TIMEOUT));
    } else {
        write(STDOUT_FILENO, buf, n);
    }

    return 0;
}
