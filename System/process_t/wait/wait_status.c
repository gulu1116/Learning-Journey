#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void sys_err(const char *str)
{
    perror(str);
    exit(1);
}

int main(int argc, char *argv[])
{
    int status = 0;
    pid_t wpid = 0;

    pid_t pid = fork();
    if (pid == -1) {
        // fork失败时调用自定义错误处理
        sys_err("fork err");
    } else if(pid == 0) {
        // 子进程逻辑
        printf("I'm child pid = %d\n", getpid());
        
        execl("./abnor", "abnor", NULL);
        perror("execl err");
        exit(1);

        sleep(3);
        exit(73);   // 子进程自定义退出码

    } else {
        // 父进程逻辑：等待并解析子进程状态
        wpid = wait(&status);
        if (wpid == -1) {
            sys_err("wait err");
        }

        // 判断子进程退出状态（正常退出 / 信号终止）
        if (WIFEXITED(status)) {
            printf("I'm parent, pid = %d | child %d exited normally, exit code = %d\n",
                   getpid(), wpid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("I'm parent, pid = %d | child %d killed by signal %d\n",
                   getpid(), wpid, WTERMSIG(status));
        }
    }

    return 0;
}