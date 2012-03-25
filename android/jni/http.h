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

#include <string>
#include <map>

using std::string;
using std::map;

typedef enum{ UNSENT=0, OPENED=1, HEADERS_RECEIVED=2, LOADING=3, DONE=4 } HTTP_STATE;


class CXMLHttpRequest
{
	private:
		bool m_post;
		string m_url;
		bool m_async;
		map<string, string> m_headers;
	
	public:
		int status;
		int readyState;
		string responseText;
		v8::Persistent<v8::Function> onreadystatechange;

	public:
		CXMLHttpRequest();
		~CXMLHttpRequest();

		JS_CLASS_EXPORT_DEF(CXMLHttpRequest)

		void open(const string &method, const string &url, bool async=true);
		void send(const string &postData);
		void setRequestHeader(const string &key, const string &value);
};
