#include <iostream>
using namespace std;

constexpr int add(int a, int b) {
    return a + b;
}

int main() {
    // 合法：编译期计算，结果为8
    constexpr int sum1 = add(3, 5);
    cout << sum1 << endl;
    
    // 合法：运行时计算，结果为8（仍合法），
    // 印证了constexpr不是"仅编译期执行"的标志，而是"允许编译期执行"的标志
    int x = 3, y = 5;
    int sum2 = add(x, y);
    cout << sum2 << endl;
    
    // 不合法：编译期计算，不能传入变量；
    // constexpr int sum3 = add(x, y); // 错误
    return 0;
}