#include "asyncServer.hpp"
int main() {
    AsyncServer server;
    server.setServerConfig(SOCK_TCP | SOCK_UDP);
    
    auto entryPoint = [](void* buf,int len) {
        std::string x((char*)(buf),len);
		std::cout <<"Client data : " << x << std::endl;
    };
    server.registerHandler(entryPoint);

    server.Run();
}