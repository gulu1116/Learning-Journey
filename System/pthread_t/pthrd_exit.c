
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

void sys_err(const char *str)
{
    perror(str);
    exit(1);
}

void *func(void) 
{
    pthread_exit(NULL);  // 直接退出当前线程
    return NULL;
}

void *tfn(void *arg)
{
    int i = (int)(long)arg;
    sleep(i);
    if (i == 2) {
        // exit(0);  // 退出当前线程
        // return NULL;  // 返回到调用者那里
        func();
    }
    printf("this is %dth thread, pid: %d, tid: %lu\n", i+1, getpid(), pthread_self());

    return NULL;
}

int main(int argc, char *argv[])
{
    int n = 5;
    if (argc == 2) {
        n = atoi(argv[1]);
    }

    int i, ret;
    pthread_t tid;
    for (i = 0; i < n; i++) {
        ret = pthread_create(&tid, NULL, tfn, (void *)(long)i);
        if (ret != 0) {
            fprintf(stderr, "pthread_create err: %s\n", strerror(ret));
        }
    }



    sleep(i);
    printf("this is main thread, pid: %d, tid: %lu\n", getpid(), pthread_self());

    return 0;
}