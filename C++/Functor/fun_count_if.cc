#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

class Compare {
public:
	 // 构造
    Compare(int length) : length(length) {}
    // ()运算符重载
    bool operator()(const string& str) {
        return str.length() > length;
    }
private:
    int length;
};

template<class T, class Function>
int my_count_if(T a, T b, Function function) {
    int count = 0;
    while (a != b) {
        if (function(*a)) {
            count++;
        }
        a++;
    }
    return count;
}

int main() {
    vector<string> names = {"GuLu", "iu", "AudreyHepburn", "Leonardo"};
    int res1 = count_if(names.begin(), names.end(), Compare(2)); // 阈值 2
	int res2 = my_count_if(names.begin(), names.end(), Compare(6)); // 阈值 6
    cout << res1 << endl;
    cout << res2 << endl;
    return 0;
}