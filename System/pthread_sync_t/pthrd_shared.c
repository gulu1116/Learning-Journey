#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>  // 为 time(NULL) 提供头文件

// 线程函数：循环输出 "hello world"
void *tfn(void *arg) {
    // 初始化随机数种子（线程内也可单独初始化，或主线程统一初始化）
    srand(time(NULL));  
    while (1) {
        printf("hello ");
        // 模拟随机时长操作，增加线程竞争概率
        sleep(rand() % 3);  
        printf("world\n");
        sleep(rand() % 3);
    }
    return NULL;
}

int main(void) {
    pthread_t tid;
    // 主线程初始化随机数种子（也可在子线程单独初始化）
    srand(time(NULL));  

    // 创建子线程，执行 tfn 函数
    pthread_create(&tid, NULL, tfn, NULL);  

    // 主线程循环输出 "HELLO WORLD"
    while (1) {
        printf("HELLO ");
        sleep(rand() % 3);  
        printf("WORLD\n");
        sleep(rand() % 3);
    }

    // 理论上不会执行到（因线程无限循环），实际需手动终止程序
    pthread_join(tid, NULL);  
    return 0;
}