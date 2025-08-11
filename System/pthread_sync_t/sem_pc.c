#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>

#define NUM 10

int queue[NUM];                 // 全局数组实现环形队列
sem_t blank_num, product_num;   // 空格信号量，产品信号量

void *producer(void *arg)
{
    int i = 0;

    while (1) {
        sem_wait(&blank_num);                       // 生产者将空格数--，为0则阻塞等待
        queue[i] = rand() % 1000 + 1;               // 生产一个产品
        printf("-----Produce----%d\n", queue[i]);
        sem_post(&product_num);                     // 将产品数++

        i = (i + 1) % NUM;                          // 借助下标实现环形
        sleep(rand() % 3);
    }
}

void *consumer(void *arg)
{
    int i = 0;

    while (1) {
        sem_wait(&product_num);
        printf("----Consume----%d\n", queue[i]);
        queue[i] = 0;
        sem_post(&blank_num);

        i = (i+1) % NUM;
        sleep(rand()%3);
    }
}

int main(int argc, char *argv[])
{
    pthread_t pid, cid;

    sem_init(&blank_num, 0, NUM);
    sem_init(&product_num, 0, 0);

    pthread_create(&pid, NULL, producer, NULL);
    pthread_create(&cid, NULL, consumer, NULL);

    pthread_join(pid, NULL);
    pthread_join(cid, NULL);

    sem_destroy(&blank_num);
    sem_destroy(&product_num);

    return 0;
}