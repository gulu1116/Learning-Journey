#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char *argv[])
{
    // 检查参数数量是否正确
    if (argc != 3) {
        fprintf(stderr, "Usage: %s src_file dest_file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 打开源文件
    int fd1 = open(argv[1], O_RDONLY);
    if (fd1 == -1) {
        perror("open src err");
        exit(1);    // 程序非正常结束
    }

    // 打开目标文件
    int fd2 = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd1 == -1) {
        perror("open dst err");
        exit(1);    // 程序非正常结束
    }

    // 循环从源文件中读数据，写到目标文件中
    // 创建用于存储读到的数据的缓冲区
    char buf[1024];
    int n = 0;
    while ((n = read(fd1, buf, 1024))) {

        if (n < 0) {
            perror("read err");
            break;
        }

        // 读多少写多少，生成目标文件
        write(fd2, buf, n);
    }

    // 操作结束，关闭文件
    close(fd1);
    close(fd2);

    return 0;
}