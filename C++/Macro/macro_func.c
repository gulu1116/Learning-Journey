#include <stdio.h>
#include <stdlib.h>

#define P(x) printf("the value of " #x " is %d\n", x);

#define XXX(n) x##n

#define PRINT_XN(n) printf("x" #n " = %d\n", x##n);

int main()
{
    P(12 + 12 + 124);
    
    int XXX(1) = 12;
    // int x##1 = 12; // 预处理器会把上面这行代码替换成这一行代码
    // int x1 = 12;

    printf("%d \n", x1);

    PRINT_XN(1);
    // printf("x" "1" " = %d\n", x##1);
    // printf("x1 = %d\n", x1);

    return 0;
}