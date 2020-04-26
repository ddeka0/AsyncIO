#include "asyncServer.hpp"
int main() {
    AsyncServer server;
    auto t100 = server.createNewTimer("T100",2);
    t100->repeatIndHandler = []() {
        std::cout <<"Retry again inside repeatInd handler" << std::endl;
    };
    t100->expiryIndHandler =[]() {
        std::cout <<"t100 has expired" << std::endl;
    };
    t100->setRepeatCount(3);
    server.startTimer(t100);
    server.Run();
}