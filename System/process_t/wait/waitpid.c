#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid, pid2, wpid;
    int flg = 0;

    // 第一次 fork 创建子进程
    pid = fork();
    // 第二次 fork（注意：此写法会让父/子进程都执行 fork，实际会创建多个进程）
    pid2 = fork();

    // 错误处理：第一次 fork 失败
    if (pid == -1) {
        perror("fork error");
        exit(1);
    } 
    // 子进程 1 分支（pid == 0）
    else if (pid == 0) {
        printf("I'm process child, pid = %d\n", getpid());
        sleep(5);
        exit(4);
    } 
    // 父进程分支（pid > 0）
    else {
        do {
            // 非阻塞等待子进程：WNOHANG 使 waitpid 不阻塞，立即返回
            wpid = waitpid(pid, NULL, WNOHANG);
            // 打印等待结果与标志位，观察轮询过程
            printf("---wpid = %d ------ %d\n", wpid, flg++);

            // 若子进程未退出（wpid == 0）
            if (wpid == 0) {
                printf("NO child exited\n");
                sleep(1);  // 休眠 1 秒后继续轮询
            }
        // 当 wpid != 0 时退出循环（子进程已退出或出错）
        } while (wpid == 0);

        // 判断是否回收了指定子进程（pid）
        if (wpid == pid) {
            printf("I'm parent, I catched child process, pid = %d\n", wpid);
        } else {
            printf("other...\n");
        }
    }

    return 0;
}



/*
    双重 fork 导致进程爆炸：

        第一个 fork() 创建子进程（Child1）
        第二个 fork() 会被父进程和 Child1 同时执行，导致：

            父进程创建新子进程（Child3）
            Child1 创建孙子进程（Child2）
            最终产生 4 个进程（1父 + 2子 + 1孙）

    waitpid 使用错误：

        父进程只等待 Child1（通过 pid 指定）
        Child3（父进程创建的第二个子进程）也执行了父进程分支的代码
        Child3 尝试等待 Child1（非直接子进程）导致 waitpid 返回 -1 
    
    ---wpid = -1 ------ 0  // Child3 等待失败
    I'm process child, pid = 6040  // Child1 输出
    other...  // Child3 的 "else" 输出
    I'm process child, pid = 6042  // Child2 输出
*/