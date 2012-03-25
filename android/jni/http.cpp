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

#include "global.h"
#include "network.h"
#include "js.h"

#include "http.h"

static v8::Handle<v8::Value> JS_open( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	v8::Local<v8::Object> self = args.Holder();
	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
	CXMLHttpRequest *request = static_cast<CXMLHttpRequest *>(wrap->Value());

	if( !request || argc < 2 )
	{
		return v8::Undefined();
	}

	string method;
	method = ValueCast::Cast(method, args[0]);

	string url;
	url = ValueCast::Cast(url, args[1]);
	
	bool async = true;
	if( argc >= 3 )
	{
		async = args[2]->BooleanValue();
	}

	request->open(method, url, async);

	return v8::Undefined();
}

static v8::Handle<v8::Value> JS_send( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	v8::Local<v8::Object> self = args.Holder();
	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
	CXMLHttpRequest *request = static_cast<CXMLHttpRequest *>(wrap->Value());

	if( !request )
	{
		return v8::Undefined();
	}

	string post;

	if( argc >= 1 )
	{
		post = ValueCast::Cast(post, args[0]);
	}
	
	request->send(post);

	return v8::Undefined();
}

JS_PROPERTY_READONLY(CXMLHttpRequest, Int32, status)
JS_PROPERTY_READONLY(CXMLHttpRequest, Int32, readyState)
JS_PROPERTY_READONLY(CXMLHttpRequest, String, responseText)
JS_PROPERTY(CXMLHttpRequest, Function, onreadystatechange)

static JSStaticValue jsStaticValues[] = {
	JS_PROPERTY_READONLY_DEF(status),
	JS_PROPERTY_READONLY_DEF(readyState),
	JS_PROPERTY_READONLY_DEF(responseText),
	JS_PROPERTY_DEF(onreadystatechange),
	{0, 0, 0}
};

static JSStaticFunction jsStaticFunctions[] = {
	JS_FUNCTION_DEF(open),
	JS_FUNCTION_DEF(send),
	{0, 0}
};

JS_CLASS_EXPORT(CXMLHttpRequest, XMLHttpRequest)

static void xmlHttpRequestCallback(Downloader *downloader, void *arg)
{
	//LOG("http callback");
	CXMLHttpRequest *request = (CXMLHttpRequest *)arg;
	request->readyState = DONE;
	request->status = downloader->status();
	if( request->status == 200 )
	{
		request->responseText = (char *)downloader->receivedData();
	}

	if( !request->onreadystatechange.IsEmpty() )
	{
		//LOG("http onreadystatechange callback");
		CV8Context::getInstance()->callJSFunction(request->onreadystatechange, 0, 0);
	}

	delete downloader;
}

CXMLHttpRequest::CXMLHttpRequest()
{
	readyState = UNSENT;
	status = -1;

	m_async = true;
}

CXMLHttpRequest::~CXMLHttpRequest()
{
}

void CXMLHttpRequest::open(const string &method, const string &url, bool async)
{
	m_method = method;
	m_url = url;
	m_async = async;
}

void CXMLHttpRequest::send(const string &post)
{
	string fullPath = CV8Context::getInstance()->path() + m_url;
	if( m_async )
	{
		AsyncDownloadQueue::getInstance()->downloadURL(&fullPath, xmlHttpRequestCallback, this, &post, NULL);
	}
	else
	{
		Downloader downloader(&fullPath, false, true, xmlHttpRequestCallback, this, &post, NULL);
		downloader.start();
	}
}
