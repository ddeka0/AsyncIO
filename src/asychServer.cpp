#include "asychServer.hpp"

#define PORT	10200
#define HOST	"127.0.0.1"

AsyncServer::AsyncServer() {
	std::cout << "AsyncServer instance Created" << std::endl;
}

AsyncServer::~AsyncServer() {
    io_uring_queue_exit(ring_.get());
};

// we need to call this Run() method from ecterna module
void AsyncServer::Run() {
	struct sockaddr_in saddr;
	int sockfd_, ret;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(PORT);
	sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd_ < 0) {
		perror("socket");
		return;
	}
	ret = bind(sockfd_, (struct sockaddr *)&saddr, sizeof(saddr));
	if (ret < 0) {
		perror("bind");
        return;
	}
    this->sockfd = sockfd_;
    this->ring_ = new io_uring();
    std::cout <<"UDP server socket created " << sockfd_ << std::endl;
    HandleRequest();
}

void AsyncServer::InitiateRequest() {

	auto initAsyncReq_READV = [this](struct io_uring_sqe* sqe,int fd,
        struct iovec* iovecs) {
		io_uring_prep_rw(IORING_OP_READV, sqe, fd, iovecs, 1, 0);
	};
	auto invoker_PrintMsg = [this](struct iovec* iov) {

		return true;
	};
	new StateMgmt<decltype(initAsyncReq_READV),decltype(invoker_PrintMsg)>
	(
        this->ring_,
        (int)IORING_OP_READV,
        this->sockfd,
        (__u32)1,
        (__u64)0,
        this,
        initAsyncReq_READV,
        invoker_PrintMsg
	);
}

void AsyncServer::HandleRequest() {
	InitiateRequest();
    // TODO LOOP
}