#include <memory>
#include <iostream>
using namespace std;

// 尾置返回类型
// 完全等价 shared_ptr<int> func()
auto func() -> shared_ptr<int> {
    // 使用 make_shared 动态分配内存
    auto sptr = make_shared<int>(10);
    
    cout << "函数中: " << *sptr << endl;
    return sptr; // 返回 shared_ptr
}

int main() {
    // 接收返回的 shared_ptr
    shared_ptr<int> sptr1 = func();
    
    // 共享所有权
    shared_ptr<int> sptr2 = sptr1;
    
    // 获取原始指针
    int* ptr = sptr1.get();
    
    // 输出信息
    cout << "*sptr1: " << *sptr1 << endl;
    cout << "*sptr2: " << *sptr2 << endl;
    
    // 释放一个 shared_ptr
    sptr1 = nullptr;
    cout << "sptr1释放后，*ptr: " << *ptr << endl; // 此时对象仍存在
    
    // 释放最后一个 shared_ptr
    sptr2 = nullptr;
    cout << "sptr2释放后，*ptr: " << *ptr << endl; // 危险！对象已释放
    
    return 0;
}