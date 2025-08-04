#include <stdio.h>

int add(int a, int b);
int sub(int a, int b);
int mul(int a, int b);

int main(int argc, char *argv[]) {

    printf("%d + %d = %d\n", 1, 2, add(1, 2));
    printf("%d - %d = %d\n", 10, 2, add(10, 2));
    printf("%d * %d = %d\n", 10, 2, add(10, 2));

    return 0;
}