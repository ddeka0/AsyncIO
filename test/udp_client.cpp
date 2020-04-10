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

#define PORT	10200
#define HOST	"127.0.0.1"

int main() {
	struct sockaddr_in saddr;
	int sockfd, ret;

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(PORT);
	inet_pton(AF_INET, HOST, &saddr.sin_addr);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return 1;
	}
    
    char str[] = "Hello IO_URING(UDP)";
    auto n = sendto(sockfd, str, strlen(str), 
        MSG_CONFIRM, (const struct sockaddr *) &saddr,  
            sizeof(saddr)); 
    std::cout << n << " bytes sent to server" << std::endl;
}