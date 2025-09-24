#include <iostream>
using namespace std;

class Compare {
public:
    bool operator()(int a, int b) {
        return a > b;
    }
};

int main() {
    Compare compare;
    // 使用仿函数的三种方式
    cout << compare.operator()(12, 34) << endl; // 显示调用重载函数
    cout << compare(34, 12) << endl;            // 对象的运算符重载调用，常用方式
    cout << Compare()(12, 34) << endl;          // 匿名对象调用重载函数
    return 0;
}