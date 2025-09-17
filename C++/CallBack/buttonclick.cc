#include <iostream>
#include <functional>  // 引入std::function
using namespace std;

// 模拟一个按钮类
class Button {
private:
    // 存储回调函数，表示"接受void，返回void"的函数
    function<void()> callback;
    
public:
    // 设置回调函数（接收一个函数作为参数）
    void setOnClickListener(function<void()> func)
    {
        callback = func;
    }
    
    // 模拟按钮被点击
    void click()
    {
        cout << "按钮被点击了！" << endl;
        if(callback != nullptr) {
            callback();  // 触发回调
        } else {
            cout << "未设置回调函数" << endl;
        }
    }
};

// 定义具体的回调函数（用户自己实现）
void onButtonClick()
{
    cout << "用户处理点击事件：打开对话框" << endl;
}

// 另一个回调函数
void showMessage()
{
    cout << "显示欢迎信息" << endl;
}

int main()
{
    Button btn;
    
    // 设置回调函数
    btn.setOnClickListener(onButtonClick);
    btn.click();  // 模拟点击
    
    // 更改回调函数
    btn.setOnClickListener(showMessage);
    btn.click();  // 再次点击
    
    // 使用lambda表达式作为回调
    btn.setOnClickListener([]() {
        cout << "使用Lambda表达式处理点击事件" << endl;
    });
    btn.click();  // 再次点击
    
    return 0;
}