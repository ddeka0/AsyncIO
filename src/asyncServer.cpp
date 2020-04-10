#include "asyncServer.hpp"

#define PORT_UDP	10200
#define PORT_TCP	10201
#define PORT_SCTP	10202

#define HOST		"127.0.0.1"

AsyncServer::AsyncServer() {
	std::cout << "AsyncServer instance Created" << std::endl;
}

AsyncServer::~AsyncServer() {
	std::cout <<"AsyncServer desctructed" << std::endl;
	// io_uring_queue_exit(ring_);
};

void AsyncServer::setServerConfig(uint8_t _types) {

	this->server_type = _types;
}
// we need to call this Run() method from ecterna module
void AsyncServer::Run() {
	struct sockaddr_in saddr_tcp,saddr_udp,saddr_sctp;
	int ret;

	memset(&saddr_tcp, 0, sizeof(saddr_tcp));
	saddr_tcp.sin_family = AF_INET;
	saddr_tcp.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr_tcp.sin_port = htons(PORT_TCP);

	memset(&saddr_udp, 0, sizeof(saddr_udp));
	saddr_udp.sin_family = AF_INET;
	saddr_udp.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr_udp.sin_port = htons(PORT_UDP);

	memset(&saddr_sctp, 0, sizeof(saddr_sctp));
	saddr_sctp.sin_family = AF_INET;
	saddr_sctp.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr_sctp.sin_port = htons(PORT_SCTP);

	if(this->server_type & SOCK_TCP) {
		sockfd_tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	}
	if(this->server_type & SOCK_UDP) {
		sockfd_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}
	if(this->server_type & SOCK_SCTP) {
		sockfd_sctp = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	}
	
	if (sockfd_tcp < 0 || sockfd_udp < 0 || sockfd_sctp < 0) {
		perror("socket");
		// return;
	}
	
	int32_t val = 1;

	if(sockfd_tcp > 0) {
		assert(setsockopt(sockfd_tcp, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) != -1);
		assert(setsockopt(sockfd_tcp, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != -1);
		ret = bind(sockfd_tcp, (struct sockaddr *)&saddr_tcp, sizeof(saddr_tcp));
		if (ret < 0) {
			perror("bind");
        	return;
		}
		if ((listen(sockfd_tcp, 5)) != 0) { 
			printf("Listen failed...\n"); 
			exit(0); 
		} 
		else
			printf("TCP server listening...\n");
	}
	if(sockfd_udp > 0) {
		assert(setsockopt(sockfd_udp, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) != -1);
		assert(setsockopt(sockfd_udp, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != -1);
		ret = bind(sockfd_udp, (struct sockaddr *)&saddr_udp, sizeof(saddr_udp));
		if (ret < 0) {
			perror("bind");
        	return;
		}
		printf("UDP server listening...\n");
	}
	if(sockfd_sctp > 0) {
		assert(setsockopt(sockfd_sctp, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) != -1);
		assert(setsockopt(sockfd_sctp, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != -1);
		ret = bind(sockfd_sctp, (struct sockaddr *)&saddr_sctp, sizeof(saddr_sctp));
		if (ret < 0) {
			perror("bind");
        	return;
		}
		if ((listen(sockfd_sctp, 5)) != 0) { 
			printf("Listen failed...\n"); 
			exit(0); 
		} 
		else
			printf("SCTP server listening...\n");
	}

	// create a new ring
    this->ring_ = new io_uring();
	assert(io_uring_queue_init(32, this->ring_, 0) >= 0);
    std::cout <<"Server socket created " << std::endl;
    HandleRequest();
}

void AsyncServer::InitiateRequest() {
	/******* prepare and define the handler functions for read operation ******/
	auto prep_RECV = [this](StateMgmtIntf* instance) {
		std::cout << __FUNCTION__ <<" called prep_RECV"<< std::endl;
		//std::lock_guard<std::mutex> lock(mtx);
		std::cout <<"> this : "<<this << std::endl;
		std::cout <<"> this ring : "<<this->ring_ << std::endl;
		auto sqe = io_uring_get_sqe(this->ring_);
		io_uring_prep_rw(IORING_OP_RECV, sqe,instance->getReadFd(), 
						instance->getBuffer(), READ_BUF_SIZE, 0);
		sqe->user_data = (__u64)(instance); // important
		auto ret = io_uring_submit(this->ring_);
		if (ret <= 0) {
			fprintf(stderr, "%s: sqe submit failed: %d\n", __FUNCTION__, ret);
		}
	};
	auto handle_CQE_PrintMsg = [this](StateMgmtIntf* instance,io_uring_cqe *cqe) {
		std::cout << __FUNCTION__ <<" called handle_CQE_PrintMsg"<< std::endl;
		std::cout <<"Number of bytes read : " << cqe->res << std::endl;
		std::string x((char*)(instance->getBuffer()),cqe->res/*TODO replace with ??*/);
		std::cout <<"Client data : " << x << std::endl;
		return true;
	};
	new StateMgmt<decltype(prep_RECV),decltype(handle_CQE_PrintMsg)>
	(
        this->ring_,
        this->sockfd_udp, // this instance of StateMgmt is created for UDP server read
		-1, 
        this,
        prep_RECV,
        handle_CQE_PrintMsg
	);
	/**************************************************************************/

	/***** prepare and define the handler functions for accept Operation ******/
	auto prep_ACCEPT = [this](StateMgmtIntf* instance) {
		std::cout << __FUNCTION__ <<" called prep_ACCEPT"<< std::endl;
		//std::lock_guard<std::mutex> lock(mtx);
		std::cout <<"# this : "<<this << std::endl;
		std::cout <<"# this ring : "<<this->ring_ << std::endl;
		auto sqe = io_uring_get_sqe(this->ring_);
		io_uring_prep_rw(IORING_OP_ACCEPT, sqe, this->sockfd_tcp, NULL, 0, 0);
		sqe->user_data = (__u64)(instance); // important
		auto ret = io_uring_submit(this->ring_);
		if (ret <= 0) {
			fprintf(stderr, "%s: sqe submit failed: %d\n", __FUNCTION__, ret);
		}
	};
	auto handle_CQE_AcceptHandle = [this,/*reuser params,*/ prep_RECV,handle_CQE_PrintMsg]
		(StateMgmtIntf* instance,io_uring_cqe *cqe) mutable {
		std::cout << __FUNCTION__ <<" called handle_CQE_AcceptHandle"<< std::endl;
		
		// std::lock_guard<std::mutex> lock{mtx};
		// we have to create a new instance for read operation on this new client fd
		// client fd can be access using cqe->res
		// auto _prep_RECV = [this](StateMgmtIntf* instance) {
		// 	std::cout << __FUNCTION__ <<" called prep_RECV"<< std::endl;
		// 	//std::lock_guard<std::mutex> lock(mtx);
		// 	std::cout <<"> this : "<<this << std::endl;
		// 	std::cout <<"> this ring : "<<this->ring_ << std::endl;
		// 	auto sqe = io_uring_get_sqe(this->ring_);
		// 	io_uring_prep_rw(IORING_OP_RECV, sqe,instance->getReadFd(), 
		// 					instance->getBuffer(), READ_BUF_SIZE, 0);
		// 	sqe->user_data = (__u64)(instance); // important
		// 	auto ret = io_uring_submit(this->ring_);
		// 	if (ret <= 0) {
		// 		fprintf(stderr, "%s: sqe submit failed: %d\n", __FUNCTION__, ret);
		// 	}
		// };
		// auto _handle_CQE_PrintMsg = [this](StateMgmtIntf* instance,io_uring_cqe *cqe) {
		// 	std::cout << __FUNCTION__ <<" called handle_CQE_PrintMsg"<< std::endl;
		// 	std::cout <<"Number of bytes read : " << cqe->res << std::endl;
		// 	std::string x((char*)(instance->getBuffer()),cqe->res/*TODO replace with ??*/);
		// 	std::cout <<"Client data : " << x << std::endl;
		// 	return true;
		// };		
		
		new StateMgmt<decltype(prep_RECV),decltype(handle_CQE_PrintMsg)>
		(
			this->ring_,
			cqe->res, // this instance of StateMgmt is created for TCP client fd
			-1, 
			this,
			prep_RECV, // reuse the prep_RECV function used in the UDP case
			handle_CQE_PrintMsg, // reuse the handle function for UDP CQE case
			true	// this is a TCP client fd read operation
		);
	};
	new StateMgmt<decltype(prep_ACCEPT),decltype(handle_CQE_AcceptHandle)>
	(
        this->ring_,
        -1,
        this->sockfd_tcp,
		this,
        prep_ACCEPT,
        handle_CQE_AcceptHandle
	);
	/**************************************************************************/
}

void AsyncServer::HandleRequest() {
	InitiateRequest();
    // LOOP
	while(true) {
		// Do I need to use lock here ?
		struct io_uring_cqe *cqe;
		io_uring_wait_cqe(ring_, &cqe);
		if (cqe->res == -EINVAL) {
			fprintf(stdout, "recv not supported, skipping\n");
		}
		if (cqe->res < 0) {
			fprintf(stderr, "failed cqe: %d\n", cqe->res);
		} else {
			if((StateMgmtIntf*)(cqe->user_data) != nullptr)
				((StateMgmtIntf*)(cqe->user_data))->Proceed(cqe);
			else 
				std::cerr <<"Error in the cqe!" << std::endl;
		}
		std::cout <<"One cqe done! flags : "<<cqe->flags << std::endl;
		io_uring_cqe_seen(ring_, cqe);
	}
}