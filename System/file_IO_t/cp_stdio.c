// cp_stdio.c
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {

    // 检查参数数量
    if (argc != 3) {
        fprintf(stderr, "Usage: %s src_file dest_file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 以二进制只读方式打开源文件
    FILE *src = fopen(argv[1], "rb");
    if (!src) {
        perror("fopen src");
        exit(EXIT_FAILURE);
    }

    // 以二进制写方式打开目标文件
    FILE *dest = fopen(argv[2], "wb");
    if (!dest) {
        perror("fopen dest");
        fclose(src);
        exit(EXIT_FAILURE);
    }

    int ch;
    // 一次读取/写入一个字节，效率较低
    while ((ch = fgetc(src)) != EOF) {
        if (fputc(ch, dest) == EOF) {
            perror("fputc");
            fclose(src);
            fclose(dest);
            exit(EXIT_FAILURE);
        }
    }

    // 关闭文件
    fclose(src);
    fclose(dest);
    return 0;
}
