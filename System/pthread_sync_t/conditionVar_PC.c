// 借助条件变量模拟 生产者——消费者 问题

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

// 链表作为共享数据需要被互斥量保护
struct msg {
    struct msg *next;
    int num;
};
struct msg *head;

// 静态初始化一个条件变量和一个互斥量
pthread_cond_t has_product = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


/*
pthread_cond_wait(&has_product, &lock):

    释放互斥锁 lock（允许生产者获取锁添加产品）；
    阻塞等待条件变量 has_product 的信号；
    被唤醒时，重新获取 lock 再返回。
    这保证了 “等待信号时不持有锁”，避免生产者因拿不到锁而无法唤醒消费者的死锁。

    */

void *consumer(void *p)
{
    struct msg *mp;

    for (;;) {  // 类似 while(1)

        pthread_mutex_lock(&lock);
        while (head == NULL) {  // 头指针为空，说明没有节点
            pthread_cond_wait(&has_product, &lock);
        }

        mp = head;
        head = mp->next;    // 模拟消费掉一个产品
        pthread_mutex_unlock(&lock);

        printf("-Consume %lu---%d\n", pthread_self(), mp->num);
        free(mp);   // 堆空间共享
        sleep(rand() % 3);
    }
}

void *producer(void *p)
{
    struct msg *mp;
    
    for (;;) {

        mp = malloc(sizeof(struct msg));
        mp->num = rand() % 1000 + 1;  // 模拟产生一个产品
        printf("------------Product---------%d\n", mp->num);

        pthread_mutex_lock(&lock);
        // 把产生的节点挂入链表
        mp->next = head;
        head = mp;
        pthread_mutex_unlock(&lock);

        pthread_cond_signal(&has_product);  // 唤醒等待在该条件变量上的一个线程

        sleep(rand() % 3);
    }
}


int main (int argc, char *argv[])
{
    pthread_t pid, cid;
    srand(time(NULL));

    pthread_create(&pid, NULL, producer, NULL);
    pthread_create(&cid, NULL, consumer, NULL);

    pthread_join(pid, NULL);
    pthread_join(cid, NULL);

    return 0;
}