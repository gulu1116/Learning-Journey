#include <iostream>
#include <string>
using namespace std;

// 常量表达式函数
constexpr int factorial(int n) {
    return n == 1 ? 1 : n * factorial(n - 1);
}

// 常量表达式
constexpr int num = 5;

int main() {
    // 编译期间进行计算并且返回结果
    cout << factorial(num) << endl; // 输出120
    cout << factorial(3) << endl;   // 输出6
    
    // 实参为变量时，程序运行期间计算并返回结果。
    int i = 8;
    // constexpr int result = factorial(i); // 报错；表达式必须含有常量值
    int result = factorial(i); // 运行时计算
    
    cout << result << endl;    // 输出40320
    return 0;
}