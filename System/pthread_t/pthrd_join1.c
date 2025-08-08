#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// 子线程的主体函数
void *tfn(void *arg)
{
    sleep(5);
    pthread_exit((void *)"hello");
}

int main(int argc, char *argv[])
{
    pthread_t tid;
    char *retval;  // 用来存储子进程的退出值
    // 创建子线程
    int ret = pthread_create(&tid, NULL, tfn, NULL);
    if (ret != 0) {
        fprintf(stderr, "pthread_create err: %s\n", strerror(ret));
    }

    printf("--------------------------------1\n");
    // 回收子线程退出值
    ret = pthread_join(tid, (void**)&retval);
    if (ret != 0) {
        fprintf(stderr, "pthread_join err: %s\n", strerror(ret));
    }

    printf("child thread exit with %s\n", (char *)retval);  // void* 指针是8字节

    pthread_exit((void *)0);  // 退出主线程

    return 0;  // 释放进程地址空间
}