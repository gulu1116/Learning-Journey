#include "Client.h"

// 接收消息并处理
void Client::ReceiveMessage(string data) {
    messageHandle->SetData(data);
    
    // 使用Lambda表达式作为回调
    messageHandle->ParseData([this](bool success) {
        this->OnProcessMessage(success);
    });
}

// 处理消息回调
void Client::OnProcessMessage(bool isSuccess) {
    if (isSuccess) {
        cout << "成功，转交给Server" << endl;
        // 这里可以添加将消息转发给服务器的逻辑
    } else {
        cout << "验证失败，拒绝访问" << endl;
    }
}