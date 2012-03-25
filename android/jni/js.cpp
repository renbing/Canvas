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

#include <jni.h>

#include "global.h"
#include "network.h"
#include "stringutil.h"

#include "point.h"
#include "image.h"
#include "context.h"
#include "canvas.h"
#include "timer.h"
#include "http.h"
#include "socket.h"
#include "websocket.h"
#include "audio.h"
#include "jnihelper.h"


#include "js.h"

extern v8::Persistent<v8::Function> jsMainLoop;
extern jobject g_jgl;
extern JavaVM *g_jvm;

// V8 C++导出函数
static v8::Handle<v8::Value> alert( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	if( args.Length() > 0 && args[0]->IsString() )
	{
		string s;
		s = ValueCast::Cast(s, args[0]);
		
		JniHelper jniHelper;
		JNIEnv * env = jniHelper.getEnv();
		jmethodID method = jniHelper.getMethod(g_jgl, "alert", "(Ljava/lang/String;)V");
		if( method != NULL && env != NULL )
		{
			jstring msg = env->NewStringUTF(s.c_str());
			env->CallVoidMethod(g_jgl, method, msg);
			env->DeleteLocalRef(msg);
		}

		LOG("alert: %s", s.c_str());
	}

	return v8::Undefined();
}

static v8::Handle<v8::Value> trace( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	if( args.Length() > 0 && args[0]->IsString() )
	{
		string s;
		s = ValueCast::Cast(s, args[0]);

		JniHelper jniHelper;
		JNIEnv * env = jniHelper.getEnv();
		jmethodID method = jniHelper.getMethod(g_jgl, "trace", "(Ljava/lang/String;)V");
		if( method != NULL && env != NULL )
		{
			jstring msg = env->NewStringUTF(s.c_str());
			env->CallVoidMethod(g_jgl, method, msg);
			env->DeleteLocalRef(msg);
		}

		LOG("trace: %s", s.c_str());
	}

	return v8::Undefined();
}

static v8::Handle<v8::Value> setMainLoop( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	if( args.Length() >= 1 )
	{
		if( args[0]->IsFunction() )
		{
			jsMainLoop = v8::Persistent<v8::Function>::New( v8::Local<v8::Function>::Cast(args[0]) );
		}
	}

	return v8::Undefined();
}

static v8::Handle<v8::Value> setTimeout( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	if( args.Length() >= 2 && args[0]->IsFunction() )
	{
		v8::Persistent<v8::Function> callback = v8::Persistent<v8::Function>::New( v8::Local<v8::Function>::Cast(args[0]) );
		float interval = args[1]->NumberValue();

		CTimer::getInstance()->createTimer((int)interval, callback, false);
	}

	return v8::Undefined();
}

static v8::Handle<v8::Value> setInterval( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	if( args.Length() >= 2 && args[0]->IsFunction() )
	{
		v8::Persistent<v8::Function> callback = v8::Persistent<v8::Function>::New( v8::Local<v8::Function>::Cast(args[0]) );
		float interval = args[1]->NumberValue();

		int timerid = CTimer::getInstance()->createTimer((int)interval, callback, true);
		//LOG("setInterval %d", timerid);
		return v8::Int32::New(timerid);
	}

	return v8::Undefined();
}

static v8::Handle<v8::Value> clearInterval( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	if( args.Length() >= 1 )
	{
		int timerid = args[0]->Int32Value();
		//LOG("clearInterval %d", timerid);

		CTimer::getInstance()->clearInterval(timerid);
	}

	return v8::Undefined();
}

CV8Context * CV8Context::m_instance = NULL;

CV8Context::CV8Context()
{
}

CV8Context::~CV8Context()
{
	v8::Locker locker;

	if( !m_ctx.IsEmpty() )
	{
		v8::Context::Scope contextScope(m_ctx);

		while(!v8::V8::IdleNotification()) {};
		m_ctx.Dispose();
		v8::V8::Dispose();
	}
}

CV8Context * CV8Context::getInstance()
{
	if( m_instance == NULL )
	{
		if( m_instance == NULL )
		{
			m_instance = new CV8Context();
		}
	}

	return m_instance;
}

bool CV8Context::run(const string &path)
{
	m_path = path;

	if( !m_ctx.IsEmpty() )
	{
		m_ctx.Dispose();
	}

	// 初始化V8 JavaScript环境
	v8::HandleScope handleScope;
	v8::Handle<v8::ObjectTemplate> globalTpl = v8::ObjectTemplate::New();
	
	// 注册自定义C++函数以及C++类
	// v8::FunctionTemplate SetClassName(...)
	globalTpl->Set(v8::String::New("alert"), v8::FunctionTemplate::New(alert));
	globalTpl->Set(v8::String::New("trace"), v8::FunctionTemplate::New(trace));
	globalTpl->Set(v8::String::New("setMainLoop"), v8::FunctionTemplate::New(setMainLoop));
	globalTpl->Set(v8::String::New("setTimeout"), v8::FunctionTemplate::New(setTimeout));
	globalTpl->Set(v8::String::New("setInterval"), v8::FunctionTemplate::New(setInterval));
	globalTpl->Set(v8::String::New("clearInterval"), v8::FunctionTemplate::New(clearInterval));

	globalTpl->Set(v8::String::New("Image"), CImage::exportJS());
	globalTpl->Set(v8::String::New("CanvasRenderingContext2D"), CCanvasContext::exportJS());
	globalTpl->Set(v8::String::New("XMLHttpRequest"), CXMLHttpRequest::exportJS());
	globalTpl->Set(v8::String::New("Socket"), CSocket::exportJS());
	globalTpl->Set(v8::String::New("WebSocket"), CWebSocket::exportJS());
	globalTpl->Set(v8::String::New("Audio"), CAudio::exportJS());

	m_ctx = v8::Context::New(0, globalTpl);

	v8::Context::Scope contextScope(m_ctx);

	// 添加全局canvas变量
	m_ctx->Global()->Set(v8::String::New("canvas"), CCanvas::getJSObject());
	
	do {
		string indexPath = m_path + "index.js";
		string indexContent;

		int status = StringUtil::loadContentsOfURL( indexPath, indexContent );
		LOG("index.js: %s", indexContent.c_str());

		char *pch = (char *)indexContent.c_str();
		char *newPch = pch;
		int len = 0;
		bool jsRunSuccess = true;

		vector<string> jsFileList;
		StringUtil::split(indexContent, "\n", jsFileList);

		string jsFileName, jsFilePath, jsContent;
		for( int i=0; i<jsFileList.size(); i++ )
		{
			jsFileName = StringUtil::trim( jsFileList[i] );

			if( jsFileName.empty() )
			{
				continue;
			}

			LOG("js file:%s", jsFileName.c_str());
			
			jsFilePath = m_path + jsFileName;
			status = StringUtil::loadContentsOfURL( jsFilePath, jsContent );

			//jsReadBuf.readZipFile(apkArchive, jsFileName.c_str());
			v8::TryCatch tryCatch;

			v8::Handle<v8::String> source = v8::String::New(jsContent.c_str());
			v8::Handle<v8::String> filename = v8::String::New(jsFileName.c_str());
			v8::Handle<v8::Script> script = v8::Script::Compile(source, filename);
			if( script.IsEmpty() )
			{
				logException(tryCatch);	
				jsRunSuccess = false;
				break;
			}

			v8::Handle<v8::Value> result = script->Run();
			if( result.IsEmpty() )
			{
				logException(tryCatch);	
				break;
			}
		}

		if( !jsRunSuccess )
		{
			break;
		}
		else
		{
			LOG("init run js success");
		}

		
		v8::Handle<v8::Value> jsReadyValue = m_ctx->Global()->Get(v8::String::New("documentReady"));
		if( jsReadyValue.IsEmpty() || !jsReadyValue->IsFunction() )
		{
			LOG("no documentReady func");
			break;
		}

		if( !callJSFunction(v8::Handle<v8::Function>::Cast(jsReadyValue->ToObject()), 0, 0) )
		{
			break;
		}

		return true;
	}while(false);

	return false;
}

bool CV8Context::callJSFunction(v8::Handle<v8::Function> func, int argc, v8::Handle<v8::Value> argv[])
{
	if( func.IsEmpty() )
	{
		return false;
	}

	v8::HandleScope handleScope;

	v8::Context::Scope contextScope(m_ctx);

	v8::TryCatch tryCatch;

	v8::Handle<v8::Value> result = func->Call(m_ctx->Global(), argc, argv);
	if( result.IsEmpty() )
	{
		logException(tryCatch);
		return false;
	}
	
	return true;
}

void CV8Context::logException(const v8::TryCatch &tryCatch)
{
	v8::HandleScope handleScope;

	if( !tryCatch.HasCaught() )
	{
		return;
	}

	v8::String::Utf8Value error(tryCatch.Exception());
	
	string detail;
	v8::Handle<v8::Message> message = tryCatch.Message();
	if( !message.IsEmpty() )
	{
		v8::String::Utf8Value filename(message->GetScriptResourceName());
		int linenum = message->GetLineNumber();
		v8::String::Utf8Value sourceline(message->GetSourceLine());
		int start = message->GetStartColumn();
		int end = message->GetEndColumn();

		v8::String::Utf8Value stackTrace(tryCatch.StackTrace());
			
		char buf[128];
		sprintf(buf, "column %d->%d at line %d in file ", start, end, linenum);
		detail = buf;
		detail += *filename;
	}

	string log = "javascript error: " + detail + "," + *error;

	JniHelper jniHelper;
	JNIEnv * env = jniHelper.getEnv();
	jmethodID method = jniHelper.getMethod(g_jgl, "trace", "(Ljava/lang/String;)V");
	if( method != NULL && env != NULL )
	{
		jstring msg = env->NewStringUTF(log.c_str());
		env->CallVoidMethod(g_jgl, method, msg);
		env->DeleteLocalRef(msg);
	}

	LOG("%s", log.c_str());
}

const string & CV8Context::path()
{
	return m_path;
}

const v8::Persistent<v8::Context> CV8Context::context()
{
	return m_ctx;
}

void CV8Context::clean()
{
	if( m_instance )
	{
		delete m_instance;
		m_instance = NULL;
	}
}
