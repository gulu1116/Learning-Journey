#include <stdio.h>
#include <stdlib.h>

#define SQUARE1(a) (a * a)
#define SQUARE2(a) a * a
#define MAX(a, b) (a > b ? a : b)
#define PR(x) printf("The result is %d.\n", x)

int main() {
    printf("MAX = %d\n", MAX(122, 134));
    PR(100 / SQUARE1(2));  // 输出 25，因为 100/(2*2)=25
    PR(100 / SQUARE2(2));  // 输出 100，因为 100/2*2=100
    system("pause");
    return 0;
}