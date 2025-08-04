#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


// 使用 execlp 执行 ls -l -F -a
int main(int argc, char *argv[])
{
    pid_t pid = fork();
    if (pid == 0) {
        // execl("/bin/ls", "ls", "-l", "-F", "-a", NULL);
        execl("./while", "while", "aa", "bb", "cc", "dd", NULL);
        perror("/bin/ls exec error");
        exit(1);
    } else if (pid > 0) {
        sleep(1);
        printf("parent\n");
    }

    return 0;
}