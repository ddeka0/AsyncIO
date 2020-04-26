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
#include <netdb.h>
#include <liburing.h>

using namespace std;

#define SOCK_UDP		1 << 0
#define SOCK_TCP		1 << 1
#define SOCK_SCTP		1 << 2

#define _UDP_IDX		0
#define _TCP_IDX		1
#define _SCTP_IDX		2

enum _typeStateMgmt : uint8_t {
	ACCEPT	= 0,
	UDP_READ,
	TCP_READ,
	TCP_SEND,
	UDP_SEND,
	CLIENT_FD_CLOSE,
	TIMEOUT
};
enum _clientType : uint8_t {
	TCP_CLIENT = 0,
	UDP_CLIENT,
	SCTP_CLIENT,
};
#define READ_BUF_SIZE	1024
struct client_info;
class AsyncServer;
using callBackFunction = void(*)(AsyncServer*,void*,int,client_info*);
struct client_info {
	client_info():ip(""),port(""){}
	client_info(std::string _ip,std::string _port):ip(_ip),port(_port){}
	std::string to_string() {
		return ip +":"+port;
	}
	std::string ip;
	std::string port;
	_clientType ctype;
};

struct timerInfo {
	timerInfo(std::string _tname,int _duration): 
		tname(_tname),duration(_duration) {}
	std::string tname = "";
	int duration = -1; // duration in seconds
	int repeatCount = -1;
	int remainCount = -1;
	bool isStopped = false;
	bool isExpired = false;
	void (*repeatIndHandler)() = nullptr;
	void (*expiryIndHandler)() = nullptr;
	void setRepeatCount(int n) {
		repeatCount = n;
		remainCount = n - 1;
	}
};
class AsyncServer {
public:
	AsyncServer();
	~AsyncServer();
	void Run();
	void setServerConfig(uint8_t _types);
	void registerHandler(callBackFunction handler);
	int sockfd_udp = -1;
	int sockfd_tcp = -1;
	int sockfd_sctp = -1;
	uint8_t server_type;
	std::mutex mtx;
	struct io_uring *ring_;
	std::map<std::string,int> clientFdMap;
	std::map<int,std::string> fdClientMap;
	void Send(void *buf,size_t len,client_info* client_);
	timerInfo* createNewTimer(std::string tname,int duration);
	void startTimer(timerInfo*);
	void stopTimer(timerInfo*);
	bool isSocketServer = false;
protected:
private:
	void HandleRequest();
	void InitiateRequest();
};



class StateMgmtIntf {
public:
	virtual void Proceed(io_uring_cqe *cqe = nullptr) = 0;
	virtual ~StateMgmtIntf(){};

	// the following functions are required for
	// polymorphism to work only
	// because I cant take base class of StateMgmt and cast
	// because of template
	virtual void* getBuffer() = 0;
	virtual int getReadFd() = 0;
	virtual int getAcceptFd() = 0;
	virtual sockaddr_in* getClientSockAddr() = 0;
	virtual void setClientAddr(std::string &&) = 0;
	virtual void setClientPort(std::string &&) = 0;
	virtual client_info* getClientInfo() = 0;
	virtual timerInfo* getTimerInfo() = 0;
};

template< typename InitPrepFunc, typename HandlerFunc>
class StateMgmt : public StateMgmtIntf {
public:
	explicit StateMgmt(struct io_uring *ring,int read_fd,int accept_fd,
			AsyncServer * _server, 
			InitPrepFunc& initPrepFunc, HandlerFunc & handlerFunc,uint8_t _typ,
			bool _isSend = false,client_info cli = client_info(),timerInfo* _timer = nullptr)
		:ring_(ring),
		sockfd_read(read_fd),
		sockfd_accept(accept_fd),
		status_(CREATE),
		isSend(_isSend),
		m_server(_server),
		initPrepfunc_(initPrepFunc),
		handlerfunc_(handlerFunc),
		client_(std::move(cli)),
		_ts((_typeStateMgmt)_typ),
		_timerInfo(_timer) {
		// std::cout <<"StateMgmt constructor called" << std::endl;
		std::memset(buf,sizeof(buf),0);
		//std::cout <<"Created " << (void*)(this) <<" of type "<<(int)_ts<< std::endl;
		Proceed();
	}
	virtual ~StateMgmt() {
		// std::cout <<this<<" destructed"<<std::endl;
	}
	void Proceed(io_uring_cqe *cqe = nullptr) {
		switch (status_) {
			case CREATE: {
				status_ = PROCESS;
				initPrepfunc_(this);
			}
			break;
			case PROCESS: {
				// Createa a new instance before current instance goes away
				if(!isSend && cqe->res != 0 && _ts != TIMEOUT)
				new StateMgmt<InitPrepFunc,HandlerFunc>
				(
					ring_,
					sockfd_read,
					sockfd_accept,
					m_server,
					initPrepfunc_,
					handlerfunc_,
					_ts
				);
				if(cqe->res != 0)
					handlerfunc_(this,cqe);

				if(_ts == TIMEOUT && _timerInfo->remainCount >= 0 
					&& _timerInfo->isStopped == false 
					&& _timerInfo->isExpired == false) {
					assert(_timerInfo != nullptr);
					new StateMgmt<InitPrepFunc,HandlerFunc>
					(
						ring_,
						sockfd_read,
						sockfd_accept,
						m_server,
						initPrepfunc_,
						handlerfunc_,
						_ts,
						isSend,
						client_,
						_timerInfo // this pointer wull be valid
					);
				}

				status_ = FINISH;
				
				// TODO respond logic
				this->Proceed(cqe); // fix later
			}
			break;
			case FINISH: {
				assert(status_ == FINISH);
				if(cqe->res == 0) {
					auto key = m_server->fdClientMap[this->sockfd_read];
					m_server->clientFdMap.erase(m_server->clientFdMap.find(key));
					m_server->fdClientMap.erase(m_server->fdClientMap.find(this->sockfd_read));
					close(this->sockfd_read);
					std::cout <<"file descriptor,closing it!" << std::endl;
				}
				// std::cout <<"Deleted " << (void*)(this)<<" of type "<<(int)_ts<< std::endl;
				delete this;
			}
			break;
			default:
				break;
		}
	}
	void* getBuffer() {
		return (void*)buf;
	}
	int getReadFd() {return sockfd_read; }
	int getAcceptFd() { return sockfd_accept; }
	sockaddr_in* getClientSockAddr() { return &caddr;}
	client_info* getClientInfo() {return &client_;};
	void setClientAddr(std::string && x) {client_.ip = std::move(x);}
	void setClientPort(std::string && x) {client_.port = std::move(x);}
	timerInfo* getTimerInfo() {return _timerInfo;}
private:
	struct io_uring *ring_;
	int sockfd_read; 			// only one is needed
	int sockfd_accept; 			// only one is needed
	enum CallStatus { CREATE, PROCESS, FINISH };
	CallStatus status_;
	bool isSend = false;
	AsyncServer * m_server;
	InitPrepFunc initPrepfunc_;
	HandlerFunc handlerfunc_;
	client_info client_;
	_typeStateMgmt _ts;
	timerInfo* _timerInfo;

	char buf[READ_BUF_SIZE];
	struct io_uring_sqe *sqe_;
	int op_;
	struct sockaddr_in caddr; 	// to get the client information
};