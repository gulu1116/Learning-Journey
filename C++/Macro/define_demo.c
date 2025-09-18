#include <stdio.h>
#include <stdlib.h>

// 宏定义示例1: 定义变量声明宏
#define kk float a =

// 宏定义示例2: 定义URL字符串
#define targetlistUrl "http://192.168.1.4:6401/?from=android"

int main()
{
    // 使用宏定义变量
    kk 12.3;
    printf("%f\n", a);
    
    // 打印宏定义的URL
    printf("URL: %s\n", targetlistUrl);
    
    return 0;
}