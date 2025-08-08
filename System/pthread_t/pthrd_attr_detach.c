#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

// 线程函数：循环打印计数，最后通过 pthread_exit 返回退出码
void *tfn(void *arg) {
    int n = 3;
    while (n--) {
        printf("thread count %d\n", n);
        sleep(1);
    }
    pthread_exit((void *)1);
}

int main(void) {
    pthread_t tid;
    void *tret;
    int err;

#if 0
    // 方式1：通过线程属性设置分离状态（编译时若想启用，将 #if 0 改为 #if 1 ）
    pthread_attr_t attr;
    // 初始化线程属性
    pthread_attr_init(&attr);
    // 设置线程为分离状态（创建即分离）
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    // 使用属性创建线程
    pthread_create(&tid, &attr, tfn, NULL);
#else
    // 方式2：先创建线程，再主动分离（默认编译启用此分支）
    pthread_create(&tid, NULL, tfn, NULL);
    // 调用 pthread_detach 让线程分离
    pthread_detach(tid);
#endif

    // 持续尝试 join 分离线程（演示分离线程无法被 join 的效果）
    while (1) {
        sleep(1);
        // 尝试等待线程结束并获取退出码
        err = pthread_join(tid, &tret);
        if (err != 0) {
            // join 失败（分离线程无法被 join ），打印错误信息
            fprintf(stderr, "thread_join error: %s\n", strerror(err));
        } else {
            // 若 join 成功（非分离线程场景），打印退出码
            fprintf(stderr, "thread exit code %ld\n", (long)tret);
            // 退出循环
            break;
        }
    }

    return 0;
}