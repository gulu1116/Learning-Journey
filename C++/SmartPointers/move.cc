#include <memory>
#include <iostream>
using namespace std;

int main() {
    // 创建 unique_ptr
    unique_ptr<int> uptr1(new int(42));
    
    cout << "uptr1 值: " << *uptr1 << endl;
    cout << "uptr1 地址: " << uptr1.get() << endl;
    
    // 转移所有权（使用 std::move）
    unique_ptr<int> uptr2 = move(uptr1);
    
    cout << "转移后:" << endl;
    cout << "uptr1 地址: " << uptr1.get() << endl; // 变为 nullptr
    cout << "uptr2 值: " << *uptr2 << endl;
    cout << "uptr2 地址: " << uptr2.get() << endl; // 与原 uptr1 相同
    
    // 检查 uptr1 是否为空
    if (!uptr1) {
        cout << "uptr1 已变为空" << endl;
    }
    
    return 0;
}