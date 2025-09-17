#include "ProjectHead.h"
#include "Client.h"

int main() {
    Client client;
    client.ReceiveMessage("admin,123456");

    Client client2;
    client.ReceiveMessage("gulu,123456");
    
    return 0;
}