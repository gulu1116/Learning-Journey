#pragma once
#ifndef MESSAGE_HANDLE_H
#define MESSAGE_HANDLE_H

#include "ProjectHead.h"

class MessageHandle {
private:
    string data;
    
public:
    void ParseData(function<void(bool)> callback);
    void SetData(string data);
    vector<string> Split(const string& s, char delimiter);
};

#endif