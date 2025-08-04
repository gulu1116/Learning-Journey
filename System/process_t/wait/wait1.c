#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    pid_t pid, wpid;

    pid = fork();

    if (pid == -1) {
        perror("fork error");
        exit(1);
    } else if (pid == 0) { 
        // 子进程分支
        printf("I'm process child, pid = %d\n", getpid());
        sleep(7); 
        // 子进程休眠7秒，模拟耗时操作
    } else { 
        // 父进程分支
lable:
        wpid = wait(NULL); 
        // 阻塞等待子进程退出，回收资源
        if (wpid == -1) {
            perror("wait error");
            goto lable;
        }
        printf("I'm parent, I catched child process, pid = %d\n", wpid);
    }

    return 0;
}