#pragma once
#ifndef CLIENT_H
#define CLIENT_H

#include "ProjectHead.h"
#include "MessageHandle.h"

class Client {
private:
    MessageHandle* messageHandle = new MessageHandle();
    
public:
    // 接收消息
    void ReceiveMessage(string data);
    // 回调函数
    void OnProcessMessage(bool isSuccess);
};

#endif