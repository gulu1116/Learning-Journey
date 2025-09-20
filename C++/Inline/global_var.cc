#include <iostream>
#include <string>
using namespace std;

// 全局变量，地址编译期可确定
int global_var = 42;

constexpr int* change(int* p) {
    *p = 12;
    return p;
}

int main() {
    // 合法：指向全局变量的指针是字面值类型，可用于constexpr
    constexpr int* ptr_to_global = &global_var;
    
    // 合法
    int* p = change(&global_var);
    
    cout << p << endl;           // 输出地址
    cout << ptr_to_global << endl; // 输出相同地址
    cout << global_var << endl;  // 输出12
    cout << *p << endl;          // 输出12
    
    return 0;
}