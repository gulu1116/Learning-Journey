#include <iostream>
using namespace std;

class Compare {
public:
    bool operator()(int a, int b) {
        return a > b;
    }
};

template<class Function>
void compare(int a, int b, Function function) {
    if (function(a, b))
        cout << a << " is bigger than " << b << endl;
    else
        cout << b << " is bigger than " << a << endl;
}

int main() {
    compare(12, 34, Compare());   // 使用仿函数
    compare(120, 34, Compare());
    return 0;
}