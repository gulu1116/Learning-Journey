#ifndef VECTOR3_H
#define VECTOR3_H

#include <iostream>
using namespace std;

class Vector3
{
private:
    float X;
    float Y;
    float Z;
    
public:
    Vector3(float x = 0, float y = 0, float z = 0) : X(x), Y(y), Z(z) { } // 构造函数
    void print();

    Vector3& operator()(int x, int y, int z);

    
    // 成员函数形式的运算符重载
    Vector3 operator+(const Vector3 &v2);
    
    // 友元函数形式的运算符重载
    friend Vector3 operator+(const Vector3 &v1, const Vector3 &v2);

    // 前置++
    Vector3& operator++();
    // 后置++
    Vector3 operator++(int);


    Vector3& operator+=(const Vector3 &v);
    Vector3& operator/=(const Vector3 &v);

    bool operator==(const Vector3 &v);
};

#endif