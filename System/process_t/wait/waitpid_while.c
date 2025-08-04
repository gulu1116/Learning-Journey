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
    int n = 5, i = 0;  //默认创建5个子进程
    if(argc == 2){
        n = atoi(argv[1]);
    }

    pid_t pid, wpid;
    for(i = 0; i < n; i++)
    {
        pid = fork();
        if (pid == 0) {
            break;
        }
    }

    if ( 5 == i) {  // 父进程
#if 0
        while ((wpid = wait(NULL)) != -1) { // 阻塞等待子进程结束然后回收
            printf("wait child %d\n", wpid);
        }
#elif 0
        // 使用 waitpid 阻塞等待子进程结束然后回收
        while ((wpid = waitpid(-1, NULL, 0)) != -1) { 
            printf("wait child %d\n", wpid);
        }
#else
        // 使用 waitpid 非阻塞回收子进程
        while ((wpid = waitpid(-1 /*或 0 */, NULL, WNOHANG)) != -1) { 
            if (wpid > 0) {
                // 正常回收
                printf("wait child %d\n", wpid);
            } else if (wpid == 0) {
                sleep(1);
                continue;
            } 
        }

        printf("catch all child finish\n");

#endif
    } else {
        sleep(i);
        printf("%dth child, pid = %d\n", i+1, getpid());
    }

    return 0;
}