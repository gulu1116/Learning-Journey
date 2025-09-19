#include <iostream>
using namespace std;

using FuncType = int(*)(int, int);

// 符合类型的函数
int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }
// 不符合类型的函数（参数数量不同）
int subtract(int a) { return a; }

// 函数指针类型作为参数类型，实现通用计算逻辑
int calculate(int a, int b, FuncType op) {
    return op(a, b);  // 调用函数指针指向的函数
}

int main() {
    cout << calculate(2, 3, add) << endl;       // 输出 5
    cout << calculate(2, 3, multiply) << endl;  // 输出 6
    return 0;
}