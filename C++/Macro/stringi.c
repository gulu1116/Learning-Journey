#include <stdio.h>
#include <stdlib.h>

// ## 运算符
#define XNAME(n) x##n
#define PRINT_XN(n) printf("x" #n " = %d.\n", x##n);
#define COM_FUN(type) type max_##type(type a, type b){ return a > b ? a : b; }

int main()
{
    int XNAME(1) = 12;  // 相当于 int x1 = 12;
    int XNAME(2) = 99;  // 相当于 int x2 = 99;
    
    PRINT_XN(1);  // 输出 "x1 = 12."
    PRINT_XN(2);  // 输出 "x2 = 99."
    
    COM_FUN(int);  // 生成函数: int max_int(int a, int b){ return a > b ? a : b; }
    COM_FUN(float);  // 生成函数: float max_float(float a, float b){ return a > b ? a : b; }
    
    printf("%d\n", max_int(10, 16));
    printf("%f\n", max_float(122.3f, 100.0f));
    
    return 0;
}