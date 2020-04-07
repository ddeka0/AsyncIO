#include "asyncServer.hpp"

#define PORT	10200
#define HOST	"127.0.0.1"

AsyncServer::AsyncServer() {
	std::cout << "AsyncServer instance Created" << std::endl;
}

AsyncServer::~AsyncServer() {
    io_uring_queue_exit(ring_);
};

void AsyncServer::setServerConfig(ServerSocketType_e _type) {
	this->server_type = _type;
}
// we need to call this Run() method from ecterna module
void AsyncServer::Run() {
	struct sockaddr_in saddr;
	int sockfd_, ret;

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(PORT);

	__uint8_t _proto;
	if(this->server_type == SOCK_TCP) {
		_proto = IPPROTO_TCP;
	}else if(this->server_type == SOCK_UDP) {
		_proto = IPPROTO_UDP;
	}else if(this->server_type == SOCK_SCTP) {
		_proto = IPPROTO_SCTP;
	}
	sockfd_ = socket(AF_INET, this->server_type, _proto);
	if (sockfd_ < 0) {
		perror("socket");
		return;
	}
	
	int32_t val = 1;
	assert(setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) != -1);
	assert(setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != -1);

	ret = bind(sockfd_, (struct sockaddr *)&saddr, sizeof(saddr));
	if (ret < 0) {
		perror("bind");
        return;
	}
	if(this->server_type == SOCK_STREAM) {
		if ((listen(sockfd, 5)) != 0) { 
			printf("Listen failed...\n"); 
			exit(0); 
		} 
		else
			printf("Server listening...\n");
	}
 
    this->sockfd = sockfd_;
	// create a new ring
    this->ring_ = new io_uring();
	assert(io_uring_queue_init(32, this->ring_, 0) >= 0);
    std::cout <<"Server socket created " << std::endl;
    HandleRequest();
}

void AsyncServer::InitiateRequest() {
	auto initAsyncReq_CONNECT = [this](struct io_uring_sqe* sqe,int fd,
    	void *buf,unsigned int len) {
		io_uring_prep_rw(IORING_OP_ACCEPT, sqe, fd, NULL, NULL, 0);
	};
	auto invoker_ConnectHandler = [this](int fd,void *buf) {
		auto sqe = io_uring_get_sqe(this->ring_);
		char *str = new char[1024];
		unsigned len = 1024;
		io_uring_prep_rw(IORING_OP_RECV, sqe, fd, (void*)str, len, 0);
		assert(io_uring_submit(ring_) != -1);
		buf = (void*)str;
		return true;
	};
	new StateMgmt<decltype(initAsyncReq_CONNECT),decltype(invoker_ConnectHandler)>
	(
        this->ring_,
        this->sockfd,
        this,
        initAsyncReq_CONNECT,
        invoker_ConnectHandler
	);	

	auto initAsyncReq_READV = [this](struct io_uring_sqe* sqe,int fd,
    	void *buf,unsigned int len) {
		io_uring_prep_rw(IORING_OP_RECV, sqe, fd, buf, len, 0);
	};
	auto invoker_PrintMsg = [this](void *buf) {
		std::string x((char *)buf,30);
		std::cout <<"client data : " << x << std::endl;
		return true;
	};
	new StateMgmt<decltype(initAsyncReq_READV),decltype(invoker_PrintMsg)>
	(
        this->ring_,
        this->sockfd,
        this,
        initAsyncReq_READV,
        invoker_PrintMsg
	);
}

void AsyncServer::HandleRequest() {
	InitiateRequest();
    // LOOP
	while(true) {
		struct io_uring_cqe *cqe;
		io_uring_wait_cqe(ring_, &cqe);
		if (cqe->res == -EINVAL) {
			fprintf(stdout, "recv not supported, skipping\n");
		}
		if (cqe->res < 0) {
			fprintf(stderr, "failed cqe: %d\n", cqe->res);
		}
		((StateMgmtIntf*)(cqe->user_data))->Proceed();
		io_uring_cqe_seen(ring_, cqe);
	}
}