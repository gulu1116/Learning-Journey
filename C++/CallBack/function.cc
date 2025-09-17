#include <iostream>
#include <string>
#include <functional>  // 引入std::function
#include <vector>
using namespace std;

// 定义几种不同类型的字符串处理函数
string toUpper(string str)
{
    for (auto& c : str) c = toupper(c);
    cout << "转换为大写: " << str << endl;
    return str;
}

string toLower(string str)
{
    for (auto& c : str) c = tolower(c);
    cout << "转换为小写: " << str << endl;
    return str;
}

string reverseString(string str)
{
    reverse(str.begin(), str.end());
    cout << "反转字符串: " << str << endl;
    return str;
}

string addPrefix(string str)
{
    string result = "前缀_" + str;
    cout << "添加前缀: " << result << endl;
    return result;
}

string addSuffix(string str)
{
    string result = str + "_后缀";
    cout << "添加后缀: " << result << endl;
    return result;
}

int main()
{
    // 1. 创建函数表 - 存储相同签名的函数
    function<string(string)> actions[] = {
        toUpper, 
        toLower, 
        reverseString,
        addPrefix,
        addSuffix
    };
    
    const int n = sizeof(actions) / sizeof(actions[0]);
    string testString = "HelloWorld";
    
    cout << "原始字符串: " << testString << endl;
    
    // 2. 顺序执行函数表中的所有函数
    string result = testString;
    for(int i = 0; i < n; i++)
    {
        cout << "步骤 " << (i+1) << ": ";
        result = actions[i](testString);
    }
    
    return 0;
}