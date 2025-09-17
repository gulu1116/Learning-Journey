#include <iostream>
#include <functional>
#include <vector>
using namespace std;

class Button {
    vector<function<void()>> callbacks;
public:
    void addOnClickListener(function<void()> func) { 
        callbacks.push_back(func); 
    }
    
    void click() {
        cout << "按钮被点击了..." << endl;
        for (auto& cb : callbacks) cb();
    }
};

int main() {
    Button btn;
    
    // 添加多个回调函数
    btn.addOnClickListener([]() {
        cout << "第一个回调：播放声音" << endl;
    });
    
    btn.addOnClickListener([]() {
        cout << "第二个回调：更新界面" << endl;
    });
    
    btn.addOnClickListener([]() {
        cout << "第三个回调：记录日志" << endl;
    });
    
    btn.click();  // 模拟点击
    
    return 0;
}