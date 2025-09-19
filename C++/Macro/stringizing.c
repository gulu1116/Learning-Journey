#include <stdio.h>
#include <stdlib.h>

#define PSQR(x) printf("The square of " #x " is %d. \n", ((x) * (x)))
#define ADD(A,B) printf("%d + %d = %d.\n", A, B, (A + B))
#define SUM(A,B) printf(#A" + "#B" = %d.\n", (A + B))

int main()
{
    // 下面两个语句为等价关系
    PSQR(2 + 4);
    // ANSI C字符串的串联特性将这些字符串与printf()语句的其他字符串组合，生成最终的字符串
    printf("The square of " "2 + 4" " is %d. \n", ((2 + 4) * (2 + 4)));

    ADD(12, 14); // 不使用#运算符
    SUM(12, 14); // 使用#运算符

    system("pause");
    return 0;
}