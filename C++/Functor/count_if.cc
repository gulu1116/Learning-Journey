#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

// 长度判断
bool compareLength(const string& str) {
    return str.length() > 6;
}

// 自己手写 count_if 函数
template<class T, class Function>
int my_count_if(T a, T b, Function compare) {
    int count = 0;
    while (a != b) {
        if (compare(*a)) {
            count++;
        }
        a++;
    }
    return count;
}

int main() {
    vector<string> names = {"GuLu", "iu", "AudreyHepburn", "Leonardo"};
    int res1 = count_if(names.begin(), names.end(), compareLength);
    int res2 = my_count_if(names.begin(), names.end(), compareLength);
    cout << res1 << endl;
    cout << res2 << endl;
    return 0;
}