#include "asyncServer.hpp"
int main() {
    AsyncServer server;
    server.setServerConfig(SOCK_TCP | SOCK_UDP);
    
    auto entryPoint = [](AsyncServer *_server,void* buf,int len,client_info* _client) {
        std::string x((char*)(buf),len);
		std::cout <<"Client data : " << x <<" and client info :"<<_client->ip +":"+ _client->port<< std::endl;
        char str[] = "Hello from Server!";
        _server->Send(str,strlen(str) + 1,_client);
    };
    server.registerHandler(entryPoint);

    server.Run();
}