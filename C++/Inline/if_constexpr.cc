#include <iostream>
#include <type_traits>
using namespace std;

// 重命名函数以避免与 std::type_info 冲突
template <typename T>
constexpr auto get_type_info() {
    if constexpr (is_integral<T>::value) {
        return "Integral";
    } else {
        return "Non-integral";
    }
}

int main() {
    cout << get_type_info<int>() << endl;     // 输出 "Integral"
    cout << get_type_info<double>() << endl;  // 输出 "Non-integral"
    return 0;
}