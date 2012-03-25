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

#include <libwebsockets.h>

typedef enum { CONNECTING = 0, OPEN = 1, CLOSING = 2, CLOSED = 3 } WEBSOCKET_STATE;

class CWebSocket
{
	private:
		struct libwebsocket *m_wsi;
		string m_url;
	
	public:
		unsigned int readyState;
		unsigned int bufferedAmount;
		string extensions;
		string protocol;

		v8::Persistent<v8::Function> onopen;
		v8::Persistent<v8::Function> onerror;
		v8::Persistent<v8::Function> onclose;


		v8::Persistent<v8::Function> onmessage;
		string binaryType;

	private:
		void handleError();
		
	public:
		CWebSocket(const string &url, const string &protocol);
		~CWebSocket();

		JS_CLASS_EXPORT_DEF(CWebSocket)

		string & get_url() { return m_url; };

		static struct libwebsocket_context * sharedContext();
		static void mainLoop();

		void send(const string &msg);
		void close();

		void connect();
};
