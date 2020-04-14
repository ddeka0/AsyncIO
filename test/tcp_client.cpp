#include <bits/stdc++.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
using namespace std;

#define PORT	10201
#define HOST	"127.0.0.1"

int main() {
	struct sockaddr_in saddr;
	int sockfd, ret;

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(PORT);
	inet_pton(AF_INET, HOST, &saddr.sin_addr);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return 1;
	}
    
    char str[] = "Hello IO_URING(TCP)";
    ret = connect(sockfd, (const sockaddr*)&saddr, sizeof(saddr));
	if (ret < 0) {
		perror("connect");
		return 1;
	}
    auto n = send(sockfd,str,strlen(str),0);
    std::cout << n << " bytes sent to server" << std::endl;
    char buffer[1024];
	std::memset(buffer,sizeof(buffer),0);
	read(sockfd, buffer, 1024);
	std::cout <<"Received : "<< std::string(buffer,strlen(buffer) + 1) << std::endl;
	close(sockfd);
    // getchar();
}