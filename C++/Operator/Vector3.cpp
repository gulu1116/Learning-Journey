#include "Vector3.h"
#include <iostream>
#include <cmath>
using namespace std;

// 函数调用运算符
Vector3& Vector3::operator()(int x, int y, int z) {
    this->X = x;
    this->Y = y;
    this->Z = z;
    return *this;
}

// 打印函数
void Vector3::print()
{
    cout << "(" << this->X << ", " << this->Y << ", " << Z << ")" << endl;
}

// 成员函数形式的+运算符重载
Vector3 Vector3::operator+(const Vector3 &v2)
{
    Vector3 temp;
    temp.X = this->X + v2.X;
    temp.Y = this->Y + v2.Y;
    temp.Z = this->Z + v2.Z;
    return temp;
}

// 友元函数形式的+运算符重载
Vector3 operator+(const Vector3 &v1, const Vector3 &v2)
{
    Vector3 temp;
    temp.X = v1.X + v2.X;
    temp.Y = v1.Y + v2.Y;
    temp.Z = v1.Z + v2.Z;
    return temp;
}

// 前置++
Vector3& Vector3::operator++() {
    this->X = this->X + 1;
    this->Y = this->Y + 1;
    this->Z = this->Z + 1;
    return *this;
}

// 后置++
Vector3 Vector3::operator++(int) {
    Vector3 temp = *this;  // 保存原始值
    ++(*this);             // 调用前置++
    return temp;           // 返回原始值
}


Vector3& Vector3::operator+=(const Vector3 &v) {
    this->X = this->X + v.X;
    this->Y = this->Y + v.Y;
    this->Z = this->Z + v.Z;
    return *this;
}

Vector3& Vector3::operator/=(const Vector3 &v) {
    this->X = this->X / v.X;
    this->Y = this->Y / v.Y;
    this->Z = this->Z / v.Z;
    return *this;
}


// Vector3.cpp中的实现
// bool Vector3::operator==(const Vector3 &v)
// {
//     bool b1 = this->X == v.X;
//     bool b2 = this->Y == v.Y;
//     bool b3 = this->Z == v.Z;
    
//     if (b1 && b2 && b3) {
//         return true;
//     } else {
//         return false;
//     }
// }

// 浮点数比较的改进版本（考虑精度问题）
bool Vector3::operator==(const Vector3 &v)
{
    // 使用误差范围比较浮点数
    const float epsilon = 0.001f;
    bool b1 = fabs(this->X - v.X) < epsilon;
    bool b2 = fabs(this->Y - v.Y) < epsilon;
    bool b3 = fabs(this->Z - v.Z) < epsilon;
    
    return b1 && b2 && b3;
}