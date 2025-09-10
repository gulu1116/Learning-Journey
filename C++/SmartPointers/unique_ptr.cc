#include <memory>
#include <iostream>
using namespace std;

// 创建并返回 unique_ptr
unique_ptr<int> create_unique() {
    // 方式1：使用 new 直接初始化
    unique_ptr<int> uptr(new int(10));
    
    // 方式2：使用 make_unique (C++14 推荐)
    // auto uptr = make_unique<int>(10);
    
    cout << "函数内值: " << *uptr << endl;
    return uptr; // 通过移动语义返回
}

int main() {
    // 接收返回的 unique_pt
    unique_ptr<int> uptr = create_unique();
    
    // 访问对象
    cout << "main 中值: " << *uptr << endl;
    
    // 获取原始指针（但不应该手动释放）
    int* raw_ptr = uptr.get();
    cout << "原始指针值: " << *raw_ptr << endl;
    
    // 释放所有权（返回原始指针并释放管理权）
    int* released_ptr = uptr.release();
    delete released_ptr; // 需要手动释放
    
    // 检查是否为空
    if (!uptr) {
        cout << "uptr 已释放所有权" << endl;
    }
    
    return 0;
}