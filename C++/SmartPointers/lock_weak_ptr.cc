#include <memory>
#include <iostream>
using namespace std;

int main() {
    shared_ptr<int> sptr = make_shared<int>(42);
    weak_ptr<int> wptr = sptr;
    
    // 使用 lock() 获取 shared_ptr
    if (auto shared = wptr.lock()) {
        cout << "值: " << *shared << endl; // 输出 42
    }
    
    sptr.reset(); // 释放对象
    
    if (auto shared = wptr.lock()) {
        cout << "值: " << *shared << endl; // 不会执行
    } else {
        cout << "对象已释放" << endl; // 输出此内容
    }
    
    return 0;
}