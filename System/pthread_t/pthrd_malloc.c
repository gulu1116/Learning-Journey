#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

void *tfn(void *arg)
{
    char *p = (char *)arg;
    strcpy(p, "world");
    printf("in tfn: %s\n", p);

    return NULL;
}

int main(void)
{
    char *p = malloc(10);
    bzero(p, 10);
    strcpy(p, "hello");
    printf("before pthread_create, p = %s\n", p);

    pthread_t tid;
    pthread_create(&tid, NULL, tfn, (void *)p);
    sleep(1);

    printf("after pthread_create, p = %s\n", p);
    free(p);

    return 0;
}