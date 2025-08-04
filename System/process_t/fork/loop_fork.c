#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "用法: %s <子进程数量>\n", argv[0]);
        exit(1);
    }

    int n = atoi(argv[1]);  // 获取子进程数量

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork 出错");
            exit(1);
        } else if (pid == 0) {
            // 子进程执行内容
            sleep(i);  // 休眠 i 秒
            printf("我是第 %d 个子进程，pid = %d，ppid = %d\n", i + 1, getpid(), getppid());
            exit(0);   // 子进程退出
        }

        // 父进程继续创建下一个子进程
    }

    // 父进程等待子进程结束（可选）
    sleep(n);  // 保证所有子进程输出完成
    printf("This is parent: %d\n", getpid());
    
    return 0;
}