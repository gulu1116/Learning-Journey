#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// 子线程的主体函数
void *tfn(void *arg)
{
    //sleep(5);
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

    // 设置线程分离
    ret = pthread_detach(tid);
    if (ret != 0) {
        fprintf(stderr, "pthread_detach err: %s\n", strerror(ret));
    }

    sleep(1);  // 保证子线程先执行结束

    ret = pthread_join(tid, NULL);
    printf("ret = %d\n", ret);
    if (ret != 0) {
        // perror("pthread_join err");  // 错误
        fprintf(stderr, "pthread_join err: %s\n", strerror(ret));
        exit(1);
    }


    printf("main: pid = %d, pthread_id = %lu\n", getpid(), pthread_self());

    pthread_exit((void *)0);  // 退出主线程

    return 0;  // 释放进程地址空间
}