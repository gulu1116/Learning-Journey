#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

void err_thread(int ret, char *str)
{
    if (ret != 0) {
        fprintf(stderr, "%s: %s\n", str, strerror(ret));
        pthread_exit(NULL);
    }
}

// 公共区
struct msg {
    int num;
    struct msg *next;
};
struct msg *head = NULL;

// 互斥锁、条件变量
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t has_product = PTHREAD_COND_INITIALIZER;

void *consumer(void *arg) 
{
    long id = (long)arg;
    while (1) {
        struct msg *mp;

        pthread_mutex_lock(&mutex);
        while (head == NULL) {
            pthread_cond_wait(&has_product, &mutex);
        }
        mp = head;
        head = mp->next;
        pthread_mutex_unlock(&mutex);

        printf("[Consumer %ld] consume: %d\n", id, mp->num);
        free(mp);

        sleep(rand() % 3);
    }
    return NULL;
}

void *producer(void *arg)
{
    long id = (long)arg;
    while (1) {
        struct msg *p = malloc(sizeof(struct msg));
        p->num = rand() % 1000 + 1;

        pthread_mutex_lock(&mutex);
        p->next = head;
        head = p;
        pthread_mutex_unlock(&mutex);

        printf("[Producer %ld] produce: %d\n", id, p->num);

        // 唤醒一个消费者（如果希望唤醒所有消费者可用 broadcast）
        pthread_cond_signal(&has_product);

        sleep(rand() % 3);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int ret;
    pthread_t producers[3], consumers[2]; // 3 个生产者，2 个消费者

    srand(time(NULL));

    // 创建生产者
    for (long i = 0; i < 3; i++) {
        ret = pthread_create(&producers[i], NULL, producer, (void *)i);
        err_thread(ret, "pthread_create producer");
    }

    // 创建消费者
    for (long i = 0; i < 2; i++) {
        ret = pthread_create(&consumers[i], NULL, consumer, (void *)i);
        err_thread(ret, "pthread_create consumer");
    }

    // 等待线程结束（实际上不会结束）
    for (int i = 0; i < 3; i++) pthread_join(producers[i], NULL);
    for (int i = 0; i < 2; i++) pthread_join(consumers[i], NULL);

    return 0;
}
