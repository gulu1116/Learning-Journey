#include <memory>
#include <iostream>
using namespace std;

class MyClass {
public:
    MyClass(int value) : data(value) {
        cout << "MyClass 构造函数" << endl;
    }
    
    ~MyClass() {
        cout << "MyClass 析构函数" << endl;
    }
    
    void print() {
        cout << "数据: " << data << endl;
    }

private:
    int data;
};

int main() {
    // 使用 make_shared 创建 shared_ptr，传递构造函数参数
    auto ptr = make_shared<MyClass>(42);
    
    // 调用对象方法
    ptr->print();
    
    // 不需要手动释放内存，shared_ptr 会自动管理
    return 0;
}