#include "asyncServer.hpp"
int main() {
    AsyncServer server;
    server.setServerConfig(SOCK_TCP | SOCK_UDP);
    
    auto entryPoint = [](void* buf,int len,client_info* _client) {
        std::string x((char*)(buf),len);
		std::cout <<"Client data : " << x <<" and client info :"<<_client->ip +":"+ _client->port<< std::endl;

    };
    server.registerHandler(entryPoint);

    server.Run();
}