#include "asyncServer.hpp"

int main() {
    AsyncServer server;
    server.setServerConfig(SOCK_TCP | SOCK_UDP);
    server.Run();
}