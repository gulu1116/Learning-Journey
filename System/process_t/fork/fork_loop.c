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
    if (argc != 2) {
        fprintf(stderr, "用法: %s <子进程数量>\n", argv[0]);
        exit(1);
    }

    int n = atoi(argv[1]);  // 获取子进程数量

    int i = 0;
    for (i = 0; i < n; i++) 
    {
        if (fork() == 0) 
            break;  // 循环期间子进程不参与循环
    }
    
    if (i == n) {
        // 循环从表达式结束
        sleep(n);
        printf("This is parent: %d\n", getpid());
    } else {
        sleep(i);
        printf("This is %dth child: %d\n", i+1, getpid());
    }

    return 0;
}