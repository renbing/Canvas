/***
 * -*- C++ -*-
 * -*- UTF-8 -*-
 *
 * Copyright (C),
 * @file
 * @brief
 * @author:
 * 		<name>		<email>
 * 		kins ren	kins.ren@me.com
 * @version
 * @date
 * @warning
 * @History:
 * 		<author>	<time>	<desc>
*/

#include <fcntl.h>

#include "ifaddrs.h"

#include "global.h"
#include "js.h"
#include "socket.h"

static v8::Handle<v8::Value> JS_runAsServer( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	v8::Local<v8::Object> self = args.Holder();
	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
	CSocket *sock = static_cast<CSocket *>(wrap->Value());

	if( !sock || argc < 1 || !args[0]->IsInt32() )
	{
		return handleScope.Close(v8::Boolean::New(false));
	}

	int broadcastPort = args[0]->Int32Value();

	return handleScope.Close( v8::Boolean::New( sock->runAsServer(broadcastPort) ) );
}

static v8::Handle<v8::Value> JS_runAsClient( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	v8::Local<v8::Object> self = args.Holder();
	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
	CSocket *sock = static_cast<CSocket *>(wrap->Value());

	if( !sock )
	{
		return handleScope.Close( v8::Boolean::New(false) );
	}

	return handleScope.Close( v8::Boolean::New( sock->runAsClient() ) );
}

static v8::Handle<v8::Value> JS_init( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	v8::Local<v8::Object> self = args.Holder();
	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
	CSocket *sock = static_cast<CSocket *>(wrap->Value());

	if( !sock || argc < 1 || !args[0]->IsInt32() )
	{
		return handleScope.Close( v8::Boolean::New(false) );
	}

	int listenPort = args[0]->Int32Value();

	return handleScope.Close( v8::Boolean::New( sock->init(listenPort) ) );
}

static v8::Handle<v8::Value> JS_send( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	v8::Local<v8::Object> self = args.Holder();
	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
	CSocket *sock = static_cast<CSocket *>(wrap->Value());

	if( !sock || argc < 1 )
	{
		return handleScope.Close( v8::Boolean::New(false) );
	}

	string msg;
	msg = ValueCast::Cast(msg, args[0]);

	return handleScope.Close( v8::Boolean::New( sock->send(msg.c_str()) ) );
}

JS_PROPERTY(CSocket, Function, onmessage)

static JSStaticValue jsStaticValues[] = {
	JS_PROPERTY_DEF(onmessage),
	{0, 0, 0}
};

static JSStaticFunction jsStaticFunctions[] = {
	JS_FUNCTION_DEF(init),
	JS_FUNCTION_DEF(runAsClient),
	JS_FUNCTION_DEF(runAsServer),
	JS_FUNCTION_DEF(send),
	{0, 0}
};

JS_CLASS_EXPORT(CSocket, Socket)


static const char ACK[] = "i'm here";
static bool finished = false;

static void * serverHandler(void *arg)
{
	CSocket *sock = (CSocket *)arg;
	sock->handleServer();

	return NULL;
}

static void * clientHandler(void *arg)
{
	CSocket *sock = (CSocket *)arg;
	sock->handleClient();

	return NULL;
}

static void * heartbeat(void *arg)
{
	CSocket *sock = (CSocket *)arg;

	while(!finished)
	{
		sock->broadcast();
		sleep(1);
	}

	return NULL;
}

CSocket::CSocket()
{
	m_sock = -1;
	m_broadcastPort = m_listenPort = 0;

	m_bConnected = false;
}

CSocket::~CSocket()
{
	finished = true;
	
	//LOG("close socket");
	if( m_sock != -1 )
	{
		close(m_sock);
	}
}

bool CSocket::init(int port)
{
	m_listenPort = port;

	m_myIPs.clear();
	m_broadcastIPs.clear();
	if( m_sock != -1 )
	{
		close(m_sock);
	}

	if( (m_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		LOG("create socket error");
		return false;
	}

	struct ifaddrs *ifap;
	if( getifaddrs(&ifap) != 0 )
	{
		LOG("get network interface error");
		return false;
	}
	
	struct ifaddrs *p = ifap;
	for(; p; p=p->ifa_next )
	{
		struct sockaddr_in *addr;
		uint32_t ip;

		addr = (struct sockaddr_in *)(p->ifa_broadaddr);
		if( !addr )
		{
			//LOG("not broadcast ip");
			continue;
		}

		ip = ntohl(addr->sin_addr.s_addr);

		if( (ip&0xFF) != 0xFF )
		{
			continue;
		}
		
		//LOG("broadcast IP: %u %i.%i.%i.%i\n", ip, (ip>>24)&0xFF, (ip>>16)&0xFF, (ip>>8)&0xFF, (ip>>0)&0xFF);
		m_broadcastIPs.push_back(ip);

		addr = (struct sockaddr_in *)(p->ifa_addr);
		if( !addr )
		{
			//LOG("not ip");
			continue;
		}
		
		ip = ntohl(addr->sin_addr.s_addr);
		if( ip == 0 || ip == 2130706433) // 127.0.0.1
		{
			//LOG("localhost ip");
			continue;
		}

		//LOG("myIP: %s %u %i.%i.%i.%i\n", p->ifa_name, ip, (ip>>24)&0xFF, (ip>>16)&0xFF, (ip>>8)&0xFF, (ip>>0)&0xFF);
		m_myIPs.push_back(ip);

	}
	
	freeifaddrs(ifap);

	if( m_broadcastIPs.size() <= 0 || m_myIPs.size() <= 0 )
	{
		return false;
	}

	return true;
}

bool CSocket::bindHost()
{
	if( m_sock == -1 )
	{
		return false;
	}

	int so_reuse = 1;
	if( setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &so_reuse, sizeof(so_reuse)) == -1 )
	{
		LOG("reuse socket opt error\n");
		return false;
	}
	
	/*
	int flags = fcntl(m_sock, F_GETFL, 0);
	if( fcntl(m_sock, F_SETFL, flags | O_NONBLOCK) == -1 )
	{
		LOG("set socket opt non block error\n");
		return false;
	}
	*/
	
	/*
	struct timeval tv;

	tv.tv_sec = 30;  // 30 Secs Timeout
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors

	if( setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval)) == -1 )
	{
		LOG("set socket timeout opt error");
		return false;
	}
	*/
	
	struct sockaddr_in listen;
	listen.sin_family = AF_INET;
	listen.sin_addr.s_addr = htonl(INADDR_ANY);
	listen.sin_port = htons(m_listenPort);
	
	if( bind(m_sock, (struct sockaddr *) &listen, sizeof(listen)) < 0)
	{
		LOG("socket bind error!\n");
		return false;
	}

	return true;
}

void CSocket::handleClient()
{
	socklen_t addr_len = sizeof(m_client);
	
	int recvBuffSize = 1024;
	char recvBuff[recvBuffSize];
	int recvd;
	
	while(!finished)
	{
		bzero(recvBuff, recvBuffSize);
		recvd = recvfrom(m_sock, recvBuff, recvBuffSize, 0, (struct sockaddr *)&m_client, &addr_len);
		
		if( recvd < 0 )
		{
			LOG("clientHandler recvfrom error\n");
			continue;
		}

		if( strcmp(recvBuff, ACK) == 0 )
		{
			/*
			if( m_bConnected )
			{
				continue;
			}
			*/
			m_bConnected = true;
			
			uint32_t cip = ntohl(m_client.sin_addr.s_addr);
			uint32_t cport = ntohs(m_client.sin_port);
			LOG("received server heartbeat: %i.%i.%i.%i:%i\n", (cip>>24)&0xFF, (cip>>16)&0xFF, (cip>>8)&0xFF, (cip>>0)&0xFF, cport);

			// 发回ACK确认
			this->send(ACK);
		}
		else if( m_bConnected )
		{
			// 调用onRecieve 事件
			if( !onReceivedMessage(recvBuff) )
			{
				LOG("onReceivedMessage error:%s\n", recvBuff);
			}
		}
	}
}

void CSocket::handleServer()
{
	socklen_t addr_len = sizeof(m_client);
	
	int recvBuffSize = 1024;
	char recvBuff[recvBuffSize];
	int recvd;
	
	while(!finished)
	{
		bzero(recvBuff, recvBuffSize);
		recvd = recvfrom(m_sock, recvBuff, recvBuffSize, 0, (struct sockaddr *)&m_client, &addr_len);
		
		if( recvd < 0 )
		{
			LOG("serverHandler recvfrom error\n");
			continue;
		}

		//LOG("received: %s\n", recvBuff);
		if( strcmp(recvBuff, ACK) == 0 )
		{
			uint32_t cip = ntohl(m_client.sin_addr.s_addr);
			uint32_t cport = ntohs(m_client.sin_port);
			
			bool bClient = (cport == m_listenPort);
			if( bClient )
			{
				for( int i=0; i<m_myIPs.size(); i++ )
				{
					uint32_t myIP = m_myIPs[i];
					if( cip == myIP )
					{
						bClient = false;
						break;
					}
				}
			}
			
			if( bClient )
			{
				m_bConnected = true;
				LOG("received client heartbeat: %i.%i.%i.%i:%i\n", (cip>>24)&0xFF, (cip>>16)&0xFF, (cip>>8)&0xFF, (cip>>0)&0xFF, cport);
			}
		}
		else if( m_bConnected )
		{
			// 调用onRecieve 事件
			if( !onReceivedMessage(recvBuff) )
			{
				LOG("onReceivedMessage error:%s\n", recvBuff);
			}
		}
	}
}

void CSocket::broadcast()
{
	for( int i=0; i<m_broadcastIPs.size(); i++ )
	{
		uint32_t ip = m_broadcastIPs[i];
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(ip);
		addr.sin_port = htons(m_broadcastPort);
		
		int addr_len = sizeof(struct sockaddr_in);
		int len = sendto(m_sock, ACK, strlen(ACK), 0, (struct sockaddr *) &addr, addr_len);
		if( len < 0 ) 
		{
			LOG("broadcasst error: %i.%i.%i.%i\n", (ip>>24)&0xFF, (ip>>16)&0xFF, (ip>>8)&0xFF, (ip>>0)&0xFF);
		}
		else
		{
			LOG("broadcasst succ: %i.%i.%i.%i:%d\n", (ip>>24)&0xFF, (ip>>16)&0xFF, (ip>>8)&0xFF, (ip>>0)&0xFF, m_broadcastPort);
		}
	}
}

bool CSocket::send(const char *msg)
{	
	if( !m_bConnected || !msg || strlen(msg) == 0 )
	{
		return false;
	}
	
	int snd = sendto(m_sock, msg, strlen(msg), 0,(struct sockaddr *) &m_client, sizeof(m_client));
		
	if(snd < 0)
	{
		LOG("send error: %s\n", msg);
		return false;
	}
	return true;
}

bool CSocket::runAsClient()
{
	if( m_sock == -1 )
	{
		return false;
	}

	if( !bindHost() )
	{
		return false;
	}

	pthread_create(&m_clientThread, NULL, clientHandler, (void *)this);

	return true;
}

bool CSocket::runAsServer(int broadcastPort)
{
	m_broadcastPort = broadcastPort;

	if( m_sock == -1 )
	{
		return false;
	}

	int so_broadcast = 1;
	if( setsockopt(m_sock, SOL_SOCKET, SO_BROADCAST, &so_broadcast, sizeof(so_broadcast)) == -1 )
	{
		LOG("broadcast socket opt error\n");
		return false;
	}

	if( !bindHost() )
	{
		return false;
	}

	pthread_create(&m_heartbeatThread, NULL, heartbeat, (void *)this);
	pthread_create(&m_serverThread, NULL, serverHandler, (void *)this);

	return true;
}

bool CSocket::onReceivedMessage(const char *msg)
{
	v8::Locker locker;

	v8::HandleScope handleScope;

	v8::Context::Scope contextScope(CV8Context::getInstance()->context());

	if( onmessage.IsEmpty() )
	{
		return false;
	}

	int argc = 1;

	v8::Handle<v8::Value> msgValue = v8::String::New(msg);

	v8::Handle<v8::Value> argv[] = {msgValue};

	return CV8Context::getInstance()->callJSFunction(onmessage, argc, argv);
}
