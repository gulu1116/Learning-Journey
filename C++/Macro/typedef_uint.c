#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 类型重定义
typedef unsigned char u_int8;
typedef unsigned short int u_int16;
typedef unsigned int u_int32;

// 结构体定义
typedef struct Student
{
    int score;
    char* name;
    void (*p)(struct Student);
} STU, *PSTU;

int main()
{
    // 超出允许范围的示例
    u_int8 data = 250;
    u_int16 data2 = 65537;  // 超出范围
    u_int32 data3 = 4294967297; // 超出范围
    
    printf("%u, %u, %u\n", data, data2, data3);
    
    // 结构体使用
    STU stu1;
    stu1.score = 100;
    stu1.name = (char*)malloc(128);
    strcpy(stu1.name, "GuLu");
    printf("Score = %d\n", stu1.score);
    printf("Name = %s\n", stu1.name);
    
    // 结构体指针简写
    PSTU stu2;
    stu2 = (PSTU)malloc(sizeof(STU));
    stu2->score = 99;
    stu2->name = (char*)malloc(128);
    strcpy(stu2->name, "iuiu");
    printf("Score = %d\n", stu2->score);
    printf("Name = %s\n", stu2->name);
    
    return 0;
}