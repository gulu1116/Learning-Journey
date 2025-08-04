#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/*
    STDIN_FILENO    0
    STDOUT_FILENO   1
    STDERR_FILENO   2
*/

int main()
{
    char buf[10];
    int n;

    n = read(STDIN_FILENO, buf, 10);
    if (n < 0) {
        perror("read STDIN_FILENO");
        exit(1);
    }
    write(STDIN_FILENO, buf, n);
}