#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// 子线程的主体函数
void *tfn(void *arg)
{
    printf("tfn: pid = %d, pthread_id = %lu\n", getpid(), pthread_self());
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t tid;
    // 创建子线程
    int ret = pthread_create(&tid, NULL, tfn, NULL);
    if (ret != 0) {
        fprintf(stderr, "pthread_create err: %s\n", strerror(ret));
    }

    printf("main: pid = %d, pthread_id = %lu\n", getpid(), pthread_self());
    sleep(1);  // 给子线程执行时间

    return 0;  // 释放进程地址空间
}