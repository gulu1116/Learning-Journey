#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


// 使用 execlp 执行 ls -l -F -a
int main(int argc, char *argv[])
{
    int i = 0;
    for (i = 0; i < argc; i++) 
    {
        printf("argv[%d] = %s\n", i, argv[i]);
    }

    return 0;
}