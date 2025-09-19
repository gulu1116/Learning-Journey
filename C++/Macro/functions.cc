#include <iostream>
using namespace std;

// 定义函数指针类型
using Action = int(*)(string);

// 多个同类型函数
int func1(string name)
{
    cout << name << endl;
    return 1;
}

int func2(string name)
{
    cout << name << endl;
    return 2;
}

int func3(string name)
{
    cout << name << endl;
    return 3;
}

int main()
{
    // 函数指针数组（元素类型为Action）
    Action actions[] = {func1, func2, func3};
    // 通过索引调用不同函数
    cout << actions[0]("你好 1") << endl;
    cout << actions[1]("你好 2") << endl;
    cout << actions[2]("你好 3") << endl;
    return 0;
}