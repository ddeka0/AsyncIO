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
	std::unique_ptr<io_uring_cqe> cq_;
};



class RpcStateMgmtIntf {
public:
    virtual void Proceed() = 0;
};

template<
        class Service, 
        class Request, 
        class Reply,  
        typename InitPrepFunc, 
        typename InvokerFunc>
class StateMgmt : public RpcStateMgmtIntf {
public:
    explicit StateMgmt(struct io_uring_cqe* cq,
			AsyncServer * _server, InitPrepFunc& initPrepfunc, 
			InvokerFunc & invokerfunc)
		: cq_(cq), 
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
				initPrepfunc_(/* TODO */);
			}
			break;
			case PROCESS: {

				new RpcStateMgmt<Service,Request,Reply,InitAsyncReqFunc,
				InvokerRpcFunc>
				(cq_, m_server,initPrepfunc, invokerfunc);
				
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
    struct io_uring_cqe *cq_;

	InitPrepFunc initPrepfunc_;
	InvokerFunc invokerfunc_;

	enum CallStatus { CREATE, PROCESS, FINISH };
	CallStatus status_;
	AsyncServer * m_server;
};