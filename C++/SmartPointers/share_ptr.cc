#include <memory>
#include <iostream>
using namespace std;

// 创建并返回 shared_ptr
shared_ptr<int> create_shared() {
    // 方式1：使用 new 直接初始化
    shared_ptr<int> sptr(new int(10));
    
    // 方式2：使用 make_shared (推荐，更高效)
    // auto sptr = make_shared<int>(10);
    
    cout << "函数内值: " << *sptr << endl;
    cout << "函数内引用计数: " << sptr.use_count() << endl;
    return sptr;
}

int main() {
    // 创建 shared_ptr
    shared_ptr<int> sptr1 = create_shared();
    cout << "sptr1 引用计数: " << sptr1.use_count() << endl;
    
    // 共享所有权（拷贝构造）
    shared_ptr<int> sptr2 = sptr1;
    cout << "创建 sptr2 后引用计数: " << sptr1.use_count() << endl;
    
    // 再次共享
    shared_ptr<int> sptr3 = sptr2;
    cout << "创建 sptr3 后引用计数: " << sptr1.use_count() << endl;
    
    // 获取原始指针
    int* raw_ptr = sptr1.get();
    cout << "原始指针值: " << *raw_ptr << endl;
    
    // 释放一个 shared_ptr
    sptr1.reset();
    cout << "释放 sptr1 后引用计数: " << sptr2.use_count() << endl;
    
    // 再次释放
    sptr2.reset();
    cout << "释放 sptr2 后引用计数: " << sptr3.use_count() << endl;
    
    // 最后一个 shared_ptr 释放时，对象自动销毁
    sptr3.reset();
    cout << "释放 sptr3 后引用计数: 0" << endl;
    
    // 注意：此时 raw_ptr 已成为悬空指针，不应再使用
    // cout << *raw_ptr << endl; // 危险行为！
    
    return 0;
}