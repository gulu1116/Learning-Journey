#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main() 
{
    // /dev/tty --> 当前终端设备
    // 以不阻塞方式(O_NONBLOCK)打开终端设备
    int fd = open("/dev/tty", O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open /dev/tty");
        return -1;
    }
    printf("open fd: %d\n", fd);

tryagain:

    char buf[10];
    int n = read(fd, buf, sizeof(buf));

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
