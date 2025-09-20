#include <iostream>
using namespace std;

struct Point {
    int x, y;
    constexpr Point(int a, int b) : x(a), y(b) {}
    
    // 添加一个constexpr成员函数计算距离平方
    constexpr int distanceSquared() const {
        return x * x + y * y;
    }
};

constexpr Point p1(3, 4);  // 编译期常量对象

int main() {
    // 编译期计算距离平方
    constexpr int distSq = p1.distanceSquared();
    cout << "Point p1: (" << p1.x << ", " << p1.y << ")" << endl;
    cout << "Distance squared from origin: " << distSq << endl;
    
    // 可以在编译期上下文使用
    constexpr Point p2(5, 12);
    constexpr int distSq2 = p2.distanceSquared();
    cout << "Point p2: (" << p2.x << ", " << p2.y << ")" << endl;
    cout << "Distance squared from origin: " << distSq2 << endl;
    
    // 使用constexpr对象作为数组大小
    int arr[distSq > 10 ? 10 : 5] = {0}; // 条件编译期表达式
    
    return 0;
}