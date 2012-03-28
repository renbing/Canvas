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

#include <private-libwebsockets.h>

#include "global.h"
#include "urllib.h"
#include "js.h"

#include "websocket.h"

template<>
inline v8::Handle<v8::Value> jsConstructor<CWebSocket>( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();
	if( argc < 1 )
	{
		return v8::Undefined();
	}

	if( args.IsConstructCall() )
	{
		string url;
		string protocol;

		url = ValueCast::Cast(url, args[0]);
		if( argc > 1 )
		{
			protocol = ValueCast::Cast(protocol, args[1]);
		}

		CWebSocket *obj = new CWebSocket(url, protocol);
		//LOG("websockt jsConstructor");

		v8::Persistent<v8::Object> self = v8::Persistent<v8::Object>::New(args.Holder());
		self.MakeWeak(obj, jsDestructor<CWebSocket>);
		self->SetInternalField(0, v8::External::New(obj));

		return handleScope.Close(self);
	}

	return v8::Undefined();
}


static v8::Handle<v8::Value> JS_send( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	CWebSocket *request = jsGetCObject<CWebSocket>(args.Holder());

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

JS_SIMPLE_FUNCTION(CWebSocket, close)

JS_PROPERTY_READONLY_BYFUNC(CWebSocket, String, url)
JS_PROPERTY_READONLY(CWebSocket, Int32, readyState)
JS_PROPERTY_READONLY(CWebSocket, Int32, bufferedAmount)
JS_PROPERTY(CWebSocket, Function, onopen)
JS_PROPERTY(CWebSocket, Function, onerror)
JS_PROPERTY(CWebSocket, Function, onclose)
JS_PROPERTY(CWebSocket, Function, onmessage)
JS_PROPERTY_READONLY(CWebSocket, String, extensions)
JS_PROPERTY_READONLY(CWebSocket, String, protocol)
JS_PROPERTY(CWebSocket, String, binaryType)

static JSStaticValue jsStaticValues[] = {
	JS_PROPERTY_READONLY_DEF(url),
	JS_PROPERTY_READONLY_DEF(readyState),
	JS_PROPERTY_READONLY_DEF(bufferedAmount),
	JS_PROPERTY_DEF(onopen),
	JS_PROPERTY_DEF(onerror),
	JS_PROPERTY_DEF(onclose),
	JS_PROPERTY_DEF(onmessage),
	JS_PROPERTY_READONLY_DEF(extensions),
	JS_PROPERTY_READONLY_DEF(protocol),
	JS_PROPERTY_DEF(binaryType),
	{0, 0, 0}
};

static JSStaticFunction jsStaticFunctions[] = {
	JS_FUNCTION_DEF(send),
	JS_FUNCTION_DEF(close),
	{0, 0}
};

JS_CLASS_EXPORT(CWebSocket, WebSocket)

static int callback_lws_default(struct libwebsocket_context * ctx,
					struct libwebsocket *wsi,
					enum libwebsocket_callback_reasons reason,
					void *user, void *in, size_t len)
{
	CWebSocket *cobj = (CWebSocket *)(wsi->callback_arg);

	switch (reason)
	{
		case LWS_CALLBACK_CLOSED:
		{
			cobj->readyState = CLOSED;
			CV8Context::getInstance()->callJSFunction(cobj->onclose, 0, NULL);

			LOG("connection closed");
			break;
		}
			
		case LWS_CALLBACK_CLIENT_ESTABLISHED:
		{
			cobj->readyState = OPEN;
			CV8Context::getInstance()->callJSFunction(cobj->onopen, 0, NULL);

			LOG("connection established");
			libwebsocket_callback_on_writable(ctx, wsi);
			break;
		}
			
		case LWS_CALLBACK_CLIENT_RECEIVE:
		{
			v8::HandleScope handleScope;

			v8::Context::Scope contextScope(CV8Context::getInstance()->context());

			int argc = 1;

			v8::Handle<v8::Value> msgValue = v8::String::New((char *)in);
			v8::Handle<v8::Value> argv[] = {msgValue};

			CV8Context::getInstance()->callJSFunction(cobj->onmessage, argc, argv);

			LOG("receieved: %d %s", (int)len, (char *)in);
			break;
		}

		case LWS_CALLBACK_CLIENT_WRITEABLE:
		{
			//[cobj send:@"hello"];
			//libwebsocket_callback_on_writable(ctx, wsi);
			//sleep(1);
			break;
		}
			
		default:
			break;
	}
	
	return 0;
}

static struct libwebsocket_protocols protocols[] = {
    { "lws-default-protocol", callback_lws_default, 0},
    {  NULL, NULL, 0}
};

struct libwebsocket_context * CWebSocket::sharedContext()
{
	static struct libwebsocket_context *context = NULL;
	
	if( !context )
	{
		// 加锁
		if( !context )
		{
			context = libwebsocket_create_context(CONTEXT_PORT_NO_LISTEN, NULL, protocols, 
									libwebsocket_internal_extensions, NULL, NULL, -1, -1, 0);
			if( context == NULL ) 
			{
				LOG("Creating libwebsocket context failed");
			}
		}
	}
	
	return context;
}

void CWebSocket::mainLoop()
{
	libwebsocket_service(CWebSocket::sharedContext(), 0);
}

CWebSocket::~CWebSocket()
{
	LOG("websocket destruct");
	if( m_wsi )
	{
		libwebsocket_close_and_free_session(CWebSocket::sharedContext(), m_wsi, LWS_CLOSE_STATUS_GOINGAWAY);
	}
}

static void * createConnection(void *arg)
{
	if( !arg )
	{
		return NULL;
	}

	CWebSocket *cobj = (CWebSocket *)arg;
	cobj->connect();

	return NULL;
}

CWebSocket::CWebSocket(const string &url, const string &sprotocol)
{
	m_url = url;
	protocol = sprotocol;
	
	readyState = CONNECTING;
	bufferedAmount = 0;
	
//	pthread_t tid;
//	pthread_create(&tid, NULL, createConnection, (void *)this);
	connect();
}

void CWebSocket::connect()
{
	URLUtil::URLMetaStruct meta;
	URLUtil::urlParse(m_url, meta);
	
	const char *address = meta.host.c_str();
	unsigned int port = atoi(meta.port.c_str());
	int use_ssl = 0;
	const char *path = meta.path.c_str();
	const char *host = address;
	const char *origin = address;
	int ietf_version = -1; // RFC 6455, 0 hybi-00/hixie76
	
	LOG("websockt connect");
	m_wsi = libwebsocket_client_connect(CWebSocket::sharedContext(), address, port, use_ssl,path, 
										host, origin, protocols[0].name, ietf_version, (void *)this);
	
	LOG("websockt connect finished");
	if (m_wsi == NULL)
	{
		LOG("libwebsocket default connect failed");
		handleError();
	}
}

void CWebSocket::handleError()
{
	CV8Context::getInstance()->callJSFunction(onerror, 0, NULL);
}


void CWebSocket::send(const string &msg)
{
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 + LWS_SEND_BUFFER_POST_PADDING];
	strcpy((char *)&buf[LWS_SEND_BUFFER_PRE_PADDING], msg.c_str());
	
	libwebsocket_write(m_wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING], msg.size(), LWS_WRITE_TEXT);
}

void CWebSocket::close()
{
	LOG("websocket close");
	if( m_wsi )
	{
		libwebsocket_close_and_free_session(CWebSocket::sharedContext(), m_wsi, LWS_CLOSE_STATUS_GOINGAWAY);
		m_wsi = NULL;
	}
}
