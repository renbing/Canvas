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

#pragma once

#include <pthread.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <v8.h>
#include <string>
#include <vector>

using std::string;
using std::vector;

class CSocket
{
	private:
		int m_sock;
		int m_listenPort;
		int m_broadcastPort;
		struct sockaddr_in m_client;

		vector<uint32_t> m_broadcastIPs;
		vector<uint32_t> m_myIPs;

		bool m_bConnected;

		pthread_t m_heartbeatThread;
		pthread_t m_clientThread;
		pthread_t m_serverThread;

	private:
		bool bindHost();
		bool onReceivedMessage(const char *msg);

	public:
		v8::Persistent<v8::Function> onmessage;

	public:

		JS_CLASS_EXPORT_DEF(CSocket)

		CSocket();
		~CSocket();
		void broadcast();
		void handleClient();
		void handleServer();


		bool init(int port=5123);

		bool runAsClient();
		bool runAsServer(int broadcastPort);

		bool send(const char *msg);

};
