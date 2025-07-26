// cp_syscall.c
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s src_file dest_file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int src = open(argv[1], O_RDONLY);
    if (src < 0) {
        perror("open src");
        exit(EXIT_FAILURE);
    }

    int dest = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest < 0) {
        perror("open dest");
        close(src);
        exit(EXIT_FAILURE);
    }

    char buf[BUF_SIZE];
    ssize_t n;
    while ((n = read(src, buf, BUF_SIZE)) > 0) {
        if (write(dest, buf, n) != n) {
            perror("write");
            close(src);
            close(dest);
            exit(EXIT_FAILURE);
        }
    }

    close(src);
    close(dest);
    return 0;
}
