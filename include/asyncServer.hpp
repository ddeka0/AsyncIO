#pragma once
#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>   
#include <sys/un.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <liburing.h>

using namespace std;
enum ServerSocketType_e {
	SOCK_UDP = SOCK_DGRAM,
	SOCK_TCP = SOCK_STREAM,
	SOCK_SCTP = SOCK_STREAM,
	SOCK_MAX
};

class AsyncServer {
public:
	AsyncServer();
	~AsyncServer();
	void Run();
	void setServerConfig(ServerSocketType_e _type);
protected:
private:
	void HandleRequest();
	void InitiateRequest();
	struct io_uring *ring_;
	int sockfd;
	
	ServerSocketType_e server_type;
};



class StateMgmtIntf {
public:
	virtual void Proceed() = 0;
	virtual ~StateMgmtIntf(){};
};

template< typename InitPrepFunc, typename InvokerFunc>
class StateMgmt : public StateMgmtIntf {
public:
	explicit StateMgmt(struct io_uring *ring,int fd,
			AsyncServer * _server, 
			InitPrepFunc& initPrepfunc, InvokerFunc & invokerfunc)
		:ring_(ring),fd_(fd),
		status_(CREATE),
		m_server(_server),  
		initPrepfunc_(initPrepfunc),
		invokerfunc_(invokerfunc) {
		Proceed();
	}
	virtual ~StateMgmt() {}
	void Proceed() {
		switch (status_) {
			case CREATE: {
				status_ = PROCESS;
				this->sqe_ = io_uring_get_sqe(ring_);
				initPrepfunc_(sqe_,fd_,buf,1024);
				sqe_->user_data = (__u64)(this);
				int ret = io_uring_submit(this->ring_);
				if (ret <= 0) {
					fprintf(stderr, "%s: sqe submit failed: %d\n", __FUNCTION__, ret);
				}
			}
			break;
			case PROCESS: {

				new StateMgmt<InitPrepFunc,InvokerFunc>
				(
					ring_,
					fd_,
					m_server,
					initPrepfunc_,
					invokerfunc_
				);
				
				invokerfunc_(buf);

				status_ = FINISH;
				
				// TODO respond logic
				this->Proceed(); // fix later
			}
			break;
			case FINISH: {
				assert(status_ == FINISH);
				delete this;
			}
			break;
			default:
				break;
		}
	}
private:
	struct io_uring *ring_;
	int fd_;
	enum CallStatus { CREATE, PROCESS, FINISH };
	CallStatus status_;
	
	AsyncServer * m_server;
	InitPrepFunc initPrepfunc_;
	InvokerFunc invokerfunc_;

	char buf[1024];
	struct io_uring_sqe *sqe_;
	int op_;
};