#include "Vector3.h"

int main()
{
    Vector3 v1 = Vector3(23, 12, 100);
    Vector3 v2 = Vector3(230, 100, 100);

    // 两种调用方式
    Vector3 v3 = v1 + v2;                    // 普通的运算符使用
    Vector3 v4 = v1.operator+(v2);           // 使用函数名进行调用（仅成员函数形式）
    
    v3.print();
    v4.print();

    Vector3 v5 = ++v1; // 前置++
    v5.print();
    v1.print();

    Vector3 v6 = v2++; // 后置++
    v6.print();
    v2.print();

    v1 += v2 += v3;  // 合法
    v1.print();
    v2.print();
    v3.print();

    v1(100, 200, 300);
    v2(50, 100, 100);
    v3(50, 50, 50);

    v1 += v2;    // v1 = v1 + v2
    v1.print();

    v2 /= v3;    // v2 = v2 / v3
    v2.print();

    // 使用示例
    v1(12.2f, 12, 12);
    v2(12.2f, 12, 12);

    if (v1 == v2) {
        cout << "两个向量相等" << endl;
    }
    
    return 0;
}