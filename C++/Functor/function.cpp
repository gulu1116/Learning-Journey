#include <iostream>
using namespace std;

bool compareFunc(int a, int b) {
    return a > b;
}

template<class Function>
void compare(int a, int b, Function function) {
    if (function(a, b)) {
        cout << a << " is bigger than " << b << endl;
    } else {
        cout << b << " is bigger than " << a << endl;
    }
}

int main() {
    compare(12, 34, compareFunc);
    compare(120, 34, compareFunc);
    return 0;
}