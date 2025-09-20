#include <iostream>
#include <string>
using namespace std;

inline int sum(int a, int b) {
    return a + b;
}

int main() {
    int result = sum(100, 45); 
    // 编译时可能会展开为：int result = 100 + 45;
    cout << result << endl;
    return 0;
}