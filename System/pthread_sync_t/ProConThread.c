#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define BUFFER_SIZE 10

// 定义缓冲区
char buffer[BUFFER_SIZE];

// 定义信号量
sem_t empty, full, mutex;

int in = 0, out = 0;

void* producer(void* arg) {
    int id = *((int*) arg);
    while (1) {
        // 随机生成字母
        char item;
        if (rand() % 2 == 0)
            item = 'a' + rand() % 26;
        else
            item = 'A' + rand() % 26;

        sem_wait(&empty);
        sem_wait(&mutex);

        buffer[in] = item;
        in = (in + 1) % BUFFER_SIZE;

        sem_post(&mutex);
        sem_post(&full);

        printf("Producer %d produced %c\n", id, item);
        sleep(1);
    }
}

void* consumer(void* arg) {
    int id = *((int*)arg);
    while (1) {
        sem_wait(&full);
        sem_wait(&mutex);

        char item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;

        sem_post(&mutex);
        sem_post(&empty);

        printf("Consumer %d consumed %c\n", id, item);
        sleep(1);
    }
}

int main() {
    pthread_t prod_threads[3], cons_threads[2];
    int prod_ids[3], cons_ids[2];

    // 初始化信号量
    sem_init(&empty, 0, BUFFER_SIZE);
    sem_init(&full, 0, 0);
    sem_init(&mutex, 0, 1);

    srand(time(NULL));

    // 创建生产者线程
    for (int i = 0; i < 3; i++) {
        prod_ids[i] = i+1;
        pthread_create(&prod_threads[i], NULL, producer, &prod_ids[i]);
    }

    // 创建消费者线程
    for (int i = 0; i < 2; i++) {
        cons_ids[i] = i+1;
        pthread_create(&cons_threads[i], NULL, consumer, &cons_ids[i]);
    }

    // 等待线程结束
    for (int i = 0; i < 3; i++) {
        pthread_join(prod_threads[i], NULL);
    }
    for (int i = 0; i < 2; i++) {
        pthread_join(cons_threads[i], NULL);
    }

    // 销毁信号量
    sem_destroy(&empty);
    sem_destroy(&full);
    sem_destroy(&mutex);

    return 0;
}