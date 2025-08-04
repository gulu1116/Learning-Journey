#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

// void sys_ree(const char *str) {
//     perror(str);
//     exit(1);
// }


int main(int argc, char *argv[])
{
    char *p = strerror(EINTR);
    printf("strerr: %s\n", p);
    return 0;
}