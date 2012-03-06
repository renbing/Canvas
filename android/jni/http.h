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


typedef enum{ UNSENT=0, OPENED=1, HEADERS_RECEIVED=2, LOADING=3, DONE=4 }State;

class CXMLHttpRequest
{
	private:
		string m_method;
		string m_url;
		bool m_async;
	
	public:
		int status;
		int readyState;
		string responseText;
		Persistent<Function> onreadystatechange;

	public:
		CXMLHttpRequest();
		~CXMLHttpRequest();

		JS_CLASS_EXPORT_DEF(CXMLHttpRequest)

		void open(const string &method, const string &url, bool async=true);
		void send(const string &post);
};
