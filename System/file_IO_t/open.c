#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    
    // int fd = open("./text.txt", O_RDWR);
    int fd = open("./test.out", O_RDWR | O_CREAT, 0664);

    printf("file description: %d\n", fd);

    // go on writing or reading......

    int ret = close(fd);
    printf("close return: %d\n", ret);

    return 0;
}