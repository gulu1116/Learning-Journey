#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
using namespace std;

int main()
{
    // 算术类
    cout << plus<int>()(1, 2) << endl;    // 3
    cout << minus<int>()(2, 2) << endl;    // 0
    cout << multiplies<int>()(2, 2) << endl;    // 4
    cout << divides<int>()(2, 2) << endl;    // 1
    cout << modulus<int>()(3, 2) << endl;    // 1
    cout << negate<int>()(10) << endl;    // -10

    // 关系运算类
    cout << equal_to<int>()(1, 2) << endl;    // 0
    cout << not_equal_to<int>()(2, 2) << endl;    // 0
    cout << greater<int>()(2, 2) << endl;    // 0
    cout << greater_equal<int>()(2, 2) << endl;    // 1
    cout << less<int>()(3, 2) << endl;    // 0
    cout << less_equal<int>()(10, 23) << endl;    // 1

    // 使用案例
    int array[5] = {13, 443, 24, 557, 23};
    sort(array, array + 5, greater<int>());

    // 逻辑运算
    cout << logical_and<bool>()(true, false) << endl;    // 0
    cout << logical_or<bool>()(false, false) << endl;    // 0
    cout << logical_not<bool>()(true) << endl;    // 0

    return 0;
}