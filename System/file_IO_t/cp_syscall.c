// cp_syscall.c
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 1   // 定义缓冲区大小为 1B，与标准库读写做对比

int main(int argc, char *argv[]) {
    // 检查参数数量是否正确
    if (argc != 3) {
        fprintf(stderr, "Usage: %s src_file dest_file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 以只读方式打开源文件
    int src = open(argv[1], O_RDONLY);
    if (src < 0) {
        perror("open src");
        exit(EXIT_FAILURE);
    }

    // 以写方式打开目标文件（若不存在则创建，存在则清空）
    int dest = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest < 0) {
        perror("open dest");
        close(src);
        exit(EXIT_FAILURE);
    }

    char buf[BUF_SIZE];  // 声明缓冲区
    ssize_t n;           // 实际读取的字节数
    while ((n = read(src, buf, BUF_SIZE)) > 0) {

        // 将读取的内容写入目标文件
        if (write(dest, buf, n) != n) {
            perror("write");
            close(src);
            close(dest);
            exit(EXIT_FAILURE);
        }
    }

    // 关闭文件描述符
    close(src);
    close(dest);
    return 0;
}
