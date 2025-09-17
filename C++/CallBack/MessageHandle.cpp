#include "MessageHandle.h"

// 设置数据
void MessageHandle::SetData(string data) {
    this->data = data;
}

// 分割字符串
vector<string> MessageHandle::Split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

// 解析数据并调用回调函数
void MessageHandle::ParseData(function<void(bool)> callback) {
    vector<string> datas = Split(data, ',');
    
    if (datas.size() < 2) {
        callback(false);
        return;
    }
    
    string username = datas[0];
    string password = datas[1];
    
    cout << "用户名: " << username << endl;
    cout << "密码: " << password << endl;
    
    // 简单的验证逻辑
    if (username == "admin" && password == "123456") {
        callback(true);
    } else {
        callback(false);
    }
}