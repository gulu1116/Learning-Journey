#include <iostream>
using namespace std;

// constexpr 模板函数
template <typename T>
constexpr T add(T a, T b) {
    return a + b;
}

int main() {
    // 编译期计算
    constexpr int sum = add(10, 20);
    cout << "Compile-time sum: " << sum << endl;
    
    // 运行时计算
    double d = add(1.1, 2.2);
    cout << "Run-time sum: " << d << endl;
    
    // 演示编译期计算的能力 - 用于数组大小
    constexpr int array_size = add(5, 5);
    int arr[array_size] = {0}; // 数组大小在编译期确定
    
    cout << "Array size: " << sizeof(arr)/sizeof(arr[0]) << endl;
    
    return 0;
}