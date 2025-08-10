#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int a = 10; // 全局变量

int main() {
    int b = 20; // 局部变量
    pid_t pid;
    pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // 子进程修改变量
        a = 111;
        b = 222;
        printf("son: a = %d, b = %d\n", a, b);
    } else {
        // 父进程等待子进程先运行
        sleep(1);
        printf("father: a = %d, b = %d\n", a, b);
    }

    return 0;
}
