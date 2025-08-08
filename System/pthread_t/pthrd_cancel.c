#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// 子线程的主体函数
void *tfn(void *arg)
{
    while (1) {
        printf("tfn: pid = %d, pthread_id = %lu\n", getpid(), pthread_self());
        sleep(1);
    }
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
    sleep(5);

    // 杀死子线程
    pthread_cancel(tid);
    if (ret != 0) {
        fprintf(stderr, "pthread_cancel err: %s\n", strerror(ret));
    }

    printf("main: pid = %d, pthread_id = %lu\n", getpid(), pthread_self());
    pthread_exit((void *)0);  // 退出主线程

    return 0;  // 释放进程地址空间
}