#include <iostream>
#include <string>
using namespace std;

int some_function() {
    return 999;
}

int main() {
    constexpr int max_size = 100;  // 编译期确定值，可用于数组大小等编译期上下文
    int arr[max_size];             // 合法：编译期已知大小

    // 对比 const: const 变量可能在运行时初始化
    const int runtime_val = some_function();  // 运行时确定值
    // constexpr int error = some_function(); // 错误：无法在编译期确定值
    return 0;
}