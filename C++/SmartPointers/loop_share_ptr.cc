#include <memory>
#include <iostream>
using namespace std;

struct Node {
    shared_ptr<Node> next;
    shared_ptr<Node> prev;
    // 将其中一个 shared_ptr 改为 weak_ptr 即可打破循环引用
    // weak_ptr<Node> prev;
    int value;
    
    Node(int val) : value(val) {
        cout << "Node " << val << " 创建" << endl;
    }
    
    ~Node() {
        cout << "Node " << value << " 销毁" << endl;
    }
};

int main() {
    // 创建循环引用
    shared_ptr<Node> node1 = make_shared<Node>(1);
    shared_ptr<Node> node2 = make_shared<Node>(2);
    
    node1->next = node2; // node2的引用计数变为2（node2本身和node1->next都指向它）
    node2->prev = node1; // node1的引用计数变为2（node1本身和node2->prev都指向它）
    
    // node1离开作用域，引用计数从 2 减为 1（不是 0）
    // node2离开作用域，引用计数从 2 减为 1（不是 0）

    // 即使离开作用域，引用计数也不会归零，导致内存泄漏
    return 0;
}
