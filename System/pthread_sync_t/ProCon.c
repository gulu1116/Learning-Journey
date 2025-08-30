#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

#define BUFFER_SIZE 10

typedef struct {
    char item;
    int type; // 1:小写字母, 2:大写字母
} Product;

Product buffer[BUFFER_SIZE];

sem_t empty, full, mutex, lowercase, uppercase;

int in = 0, out = 0;

void* producer(void* arg) {
    int id = *((int*)arg);
    while (1) {
        // 随机生成字母和类型
        char item;
        int type = rand() % 2 + 1;
        if (type == 1) {
            item = 'a' + rand() % 26;
        } else {
            item = 'A' + rand() % 26;
        }

        sem_wait(&empty);
        sem_wait(&mutex);

        // 生产产品
        buffer[in].item = item;
        buffer[in].type = type;
        in = (in + 1) % BUFFER_SIZE;

        // 根据产品类型更新信号量
        if (type == 1) {
            sem_post(&lowercase);
        } else {
            sem_post(&uppercase);
        }

        sem_post(&full);
        sem_post(&mutex);

        printf("Producer %d produced %c\n", id, item);
        sleep(1);
    }
}

void* consumer(void* arg) {
    int id = *((int*)arg);

    while (1) {
        if (id == 3) {
            // 消费者3：消费任意字母
            sem_wait(&full);
            sem_wait(&mutex);
            
            Product item = buffer[out];
            out = (out + 1) % BUFFER_SIZE;
            
            sem_post(&mutex);
            sem_post(&empty);
            
            printf("Consumer %d consumed(random) %c\n", id, item.item);
        } else {
            // 消费者1和2：消费特定类型字母
            if (id == 1) {
                sem_wait(&lowercase);
            } else {
                sem_wait(&uppercase);
            }
            
            sem_wait(&mutex);
            
            // 确认产品类型是否匹配
            while (buffer[out].type != id) {
                sem_post(&mutex);
                if (id == 1) {
                    sem_wait(&lowercase);
                } else {
                    sem_wait(&uppercase);
                }
                sem_wait(&mutex);
            }
            
            Product item = buffer[out];
            out = (out + 1) % BUFFER_SIZE;
            
            sem_post(&mutex);
            sem_post(&empty);

            if (id == 1) {
                printf("Consumer %d consumed(lowercase) %c\n", id, item.item);
            } else {
                printf("Consumer %d consumed(UPPERCASE) %c\n", id, item.item);
            }
        }
        sleep(1);
    }
}

int main() {
    pthread_t prod_threads[3], cons_threads[3];
    int prod_ids[3], cons_ids[3];

    // 初始化信号量
    sem_init(&empty, 0, BUFFER_SIZE);
    sem_init(&full, 0, 0);
    sem_init(&mutex, 0, 1);
    sem_init(&lowercase, 0, 0);
    sem_init(&uppercase, 0, 0);

    srand(time(NULL));

    // 创建生产者线程
    for (int i = 0; i < 3; i++) {
        prod_ids[i] = i + 1;
        pthread_create(&prod_threads[i], NULL, producer, &prod_ids[i]);
    }

    // 创建消费者线程
    for (int i = 0; i < 3; i++) {
        cons_ids[i] = i + 1;
        pthread_create(&cons_threads[i], NULL, consumer, &cons_ids[i]);
    }

    // 等待线程结束
    for (int i = 0; i < 3; i++) {
        pthread_join(prod_threads[i], NULL);
        pthread_join(cons_threads[i], NULL);
    }

    // 销毁信号量
    sem_destroy(&empty);
    sem_destroy(&full);
    sem_destroy(&mutex);
    sem_destroy(&lowercase);
    sem_destroy(&uppercase);

    return 0;
}