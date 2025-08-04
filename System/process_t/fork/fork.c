#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

// 自定义的错误打印函数
void sys_err(const char *str) {
    perror(str);
    exit(1);  // 退出程序
}

int main(int argc, char *argv[])
{
    printf("==================before fork====================\n");

    pid_t pid = fork();
    if (pid == -1) {
        sys_err("fork error");
    }

    if (pid == 0) {
        // 子进程
        printf("This is a child process: %d, parent pid: %d\n", getpid(), getppid());
    } else if (pid > 0) {
        // 父进程
        printf("This is a parent process: %d, child pid: %d, my parent pid: %d\n", getpid(), pid, getppid());
        sleep(1)
    }

    printf("==================after fork====================\n");

    return 0;
}