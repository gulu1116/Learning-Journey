#include <iostream>
using namespace std;

// 定义函数指针类型：指向 "int(int, int)" 类型的函数
// 等价于 typedef int(*FuncType)(int, int);
using FuncType = int(*)(int, int);

// 符合类型的函数
int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }
// 不符合类型的函数（参数数量不同）
int subtract(int a) { return a; }

int main()
{
    FuncType ptr;
    ptr = add;      // 合法：类型匹配
    ptr = multiply; // 合法：类型匹配
    // ptr = subtract; // 编译错误：参数列表不匹配，类型不兼容
    cout << ptr(2, 3) << endl;
    return 0;
}