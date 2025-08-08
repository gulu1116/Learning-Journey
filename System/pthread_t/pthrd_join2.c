#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

struct thrd {
    int var;
    char str[256];
};


// 子线程的主体函数
void *tfn(void *arg)
{
    // struct thrd th;
    // th.var = 100;
    // strcpy(th.str, "hello thread")
    // pthread_exit((void *)&th);
    // 定义的局部变量，函数调用结束会被释放掉

    struct thrd *tval = (struct thrd *)arg;
    tval->var = 100;
    strcpy(tval->str, "hello thread");
    pthread_exit((void *)tval);
    // return (void *)tval;  // 也可以
}

int main(int argc, char *argv[])
{
    pthread_t tid;
    
    struct thrd arg, *retval;

    // 创建子线程
    int ret = pthread_create(&tid, NULL, tfn, (void *)&arg);
    if (ret != 0) {
        fprintf(stderr, "pthread_create err: %s\n", strerror(ret));
    }

    // 回收子线程退出值
    ret = pthread_join(tid, (void**)&retval);
    if (ret != 0) {
        fprintf(stderr, "pthread_join err: %s\n", strerror(ret));
    }

    printf("child thread exit with: var = %d, str = %s\n", retval->var, retval->str);  // void* 指针是8字节

    pthread_exit((void *)0);  // 退出主线程

    return 0;  // 释放进程地址空间
}