#include <memory>
#include <iostream>
using namespace std;

class B; // 前向声明

class A {
public:
    shared_ptr<B> b_ptr; // A 持有 B 的 shared_ptr
    ~A() { cout << "A 被销毁" << endl; }
};

class B {
public:
    weak_ptr<A> a_ptr; // 将 shared_ptr 改为 weak_ptr
    ~B() { cout << "B 被销毁" << endl; }
};

int main() {
    auto a = make_shared<A>();
    auto b = make_shared<B>();
    
    a->b_ptr = b; // a 引用 b
    b->a_ptr = a; // b->a_ptr 是 weak_ptr，A 对象的引用计数不会增加（仍然是 1）
    
    cout << "程序结束" << endl;
    return 0;
    
    // 离开作用域时：
    // 1. a 的引用计数从 1 减为 0 → A 被销毁
    // 2. A 销毁后，b_ptr 被销毁，b 的引用计数从 2 减为 1
    // 3. b 的引用计数从 1 减为 0 → B 被销毁
}