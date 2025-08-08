#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>  // 为 sleep 函数提供声明

// 线程函数1：通过 return 返回退出码
void *tfn1(void *arg) {
    printf("thread 1 returning\n");
    return (void *)111;
}

// 线程函数2：通过 pthread_exit 返回退出码
void *tfn2(void *arg) {
    printf("thread 2 exiting\n");
    pthread_exit((void *)222);
}

// 线程函数3：无限循环，等待被取消
void *tfn3(void *arg) {
    while (1) {
        printf("thread 3: I'm going to die in 3 seconds ...\n");
        sleep(1);
        // pthread_testcancel(); // 取消点（若启用，可让线程在取消点响应取消）
    }
    return (void *)666;  // 实际不会执行到，因线程会被取消
}

int main(void) {
    pthread_t tid;
    void *tret = NULL;

    // 测试线程1：通过 return 返回退出码
    pthread_create(&tid, NULL, tfn1, NULL);
    pthread_join(tid, &tret);
    printf("thread 1 exit code = %ld\n\n", (long)tret);

    // 测试线程2：通过 pthread_exit 返回退出码
    pthread_create(&tid, NULL, tfn2, NULL);
    pthread_join(tid, &tret);
    printf("thread 2 exit code = %ld\n\n", (long)tret);

    // 测试线程3：通过 pthread_cancel 取消线程
    pthread_create(&tid, NULL, tfn3, NULL);
    sleep(3);            // 等待 3 秒，让线程3执行几轮输出
    pthread_cancel(tid); // 发送取消请求
    pthread_join(tid, &tret); 
    // 线程被取消时，退出码通常为 PTHREAD_CANCELED（即 (void*)-1 ，不同系统可能有差异）
    printf("thread 3 exit code = %ld\n", (long)tret);

    return 0;
}