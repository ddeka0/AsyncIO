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
class AsyncServer {
public:
	AsyncServer();
	~AsyncServer();
	void Run();
protected:
private:
	void HandleRequest();
	void InitiateRequest();
	struct io_uring *ring_;
	int sockfd;
};



class StateMgmtIntf {
public:
    virtual void Proceed() = 0;
};

template< typename InitPrepFunc, typename InvokerFunc>
class StateMgmt : public StateMgmtIntf {
public:
    explicit StateMgmt(struct io_uring *ring, int op,
            int fd, __u32 len, __u64 off, AsyncServer * _server, 
            InitPrepFunc& initPrepfunc, InvokerFunc & invokerfunc)
		:ring_(ring), op_(op), fd_(fd), len_(len), off_(off),
		status_(CREATE),
		m_server(_server),  
		initPrepfunc_(initAsyncReqfunc),
		invokerfunc_(invokerRpcfunc) {
		Proceed();
	}
	void Proceed() {
		switch (status_) {
			case CREATE: {
				status_ = PROCESS;
                struct io_uring_sqe *sqe_;
                sqe_ = io_uring_get_sqe(ring_);

				initPrepfunc_(op_,sqe_,fd_,addr_,len_,off_);
				int ret = io_uring_submit(ring_);
				if (ret <= 0) {
					fprintf(stderr, "%s: sqe submit failed: %d\n", __FUNCTION__, ret);
				}
			}
			break;
			case PROCESS: {

				new StateMgmt<InitPrepFunc,InvokerFunc>(
                    cqe_, ring_,op_, fd_,len_,off_,addr_,
                    initPrepfunc_,invokerfunc_);
				
				response_ =  invokerfunc_(request_);

				status_ = FINISH;

                // TODO respond logic
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
    int op_;
    int fd_;
    __u32 len_;
    __u64 off_;
	InitPrepFunc initPrepfunc_;
	InvokerFunc invokerfunc_;

	enum CallStatus { CREATE, PROCESS, FINISH };
	CallStatus status_;
	AsyncServer * m_server;
};