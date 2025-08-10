/* 
 * 3 个线程不定时“写”全局资源，5 个线程不定时“读”同一全局资源 
 */
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

// 全局资源
int counter;
pthread_rwlock_t rwlock;

// 写线程函数
void *th_write(void *arg)
{
    int t;
    int i = (int)(long)arg;

    while (1) {
        // 这里先临时读一下（无锁，仅示例，实际按需调整）
        t = counter;
        usleep(1000);

        // 加写锁，独占全局资源
        pthread_rwlock_wrlock(&rwlock);
        printf("=======write %d: %lu: counter=%d ++counter=%d\n", 
               i, pthread_self(), t, ++counter);
        pthread_rwlock_unlock(&rwlock);

        usleep(5000);
    }
    return NULL;
}

// 读线程函数
void *th_read(void *arg)
{
    int i = (int)(long)arg;

    while (1) {
        // 加读锁，共享全局资源
        pthread_rwlock_rdlock(&rwlock);
        printf("----------------read %d: %lu: %d\n", 
               i, pthread_self(), counter);
        pthread_rwlock_unlock(&rwlock);

        usleep(900);
    }
    return NULL;
}

int main(void)
{
    int i;
    // 线程 ID 数组，共 8 个线程（3 写 + 5 读）
    pthread_t tid[8];

    // 初始化读写锁
    pthread_rwlock_init(&rwlock, NULL);

    // 创建 3 个写线程
    for (i = 0; i < 3; i++) {
        pthread_create(&tid[i], NULL, th_write, (void *)(long)i);
    }

    // 创建 5 个读线程
    for (i = 0; i < 5; i++) {
        pthread_create(&tid[i + 3], NULL, th_read, (void *)(long)i);
    }

    // 等待所有线程结束（实际会一直运行，这里只是语法完整）
    for (i = 0; i < 8; i++) {
        pthread_join(tid[i], NULL);
    }

    // 销毁读写锁
    pthread_rwlock_destroy(&rwlock);

    return 0;
}