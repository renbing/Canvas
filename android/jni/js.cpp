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
#include "stringutil.h"

#include "point.h"
#include "image.h"
#include "context.h"
#include "canvas.h"
#include "timer.h"
#include "http.h"
#include "socket.h"


#include "js.h"

extern Persistent<Function> jsMainLoop;

// V8 C++导出函数
static Handle<Value> alert( const Arguments& args )
{
	HandleScope handleScope;

	if( args.Length() > 0 && args[0]->IsString() )
	{
		string s;
		s = ValueCast::Cast(s, args[0]);
		LOG("alert: %s", s.c_str());
	}

	return Undefined();
}

static Handle<Value> trace( const Arguments& args )
{
	HandleScope handleScope;

	if( args.Length() > 0 && args[0]->IsString() )
	{
		string s;
		s = ValueCast::Cast(s, args[0]);
		LOG("trace: %s", s.c_str());
	}

	return Undefined();
}

static Handle<Value> setMainLoop( const Arguments& args )
{
	HandleScope handleScope;

	if( args.Length() >= 1 )
	{
		if( args[0]->IsFunction() )
		{
			jsMainLoop = Persistent<Function>::New( Local<Function>::Cast(args[0]) );
		}
	}

	return Undefined();
}

static Handle<Value> setTimeout( const Arguments& args )
{
	HandleScope handleScope;

	if( args.Length() >= 2 && args[0]->IsFunction() )
	{
		Persistent<Function> callback = Persistent<Function>::New( Local<Function>::Cast(args[0]) );
		float interval = args[1]->NumberValue();

		CTimer::getInstance()->createTimer((int)interval, callback, false);
	}

	return Undefined();
}

static Handle<Value> setInterval( const Arguments& args )
{
	HandleScope handleScope;

	if( args.Length() >= 2 && args[0]->IsFunction() )
	{
		Persistent<Function> callback = Persistent<Function>::New( Local<Function>::Cast(args[0]) );
		float interval = args[1]->NumberValue();

		int timerid = CTimer::getInstance()->createTimer((int)interval, callback, true);
		//LOG("setInterval %d", timerid);
		return Int32::New(timerid);
	}

	return Undefined();
}

static Handle<Value> clearInterval( const Arguments& args )
{
	HandleScope handleScope;

	if( args.Length() >= 1 )
	{
		int timerid = args[0]->Int32Value();
		//LOG("clearInterval %d", timerid);

		CTimer::getInstance()->clearInterval(timerid);
	}

	return Undefined();
}

CV8Context * CV8Context::m_instance = NULL;

CV8Context::CV8Context()
{
}

CV8Context::~CV8Context()
{
	if( !m_ctx.IsEmpty() )
	{
		m_ctx.Dispose();
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
	HandleScope handleScope;
	Handle<ObjectTemplate> globalTpl = ObjectTemplate::New();
	
	// 注册自定义C++函数以及C++类
	globalTpl->Set(String::New("alert"), FunctionTemplate::New(alert));
	globalTpl->Set(String::New("trace"), FunctionTemplate::New(trace));
	globalTpl->Set(String::New("setMainLoop"), FunctionTemplate::New(setMainLoop));
	globalTpl->Set(String::New("setTimeout"), FunctionTemplate::New(setTimeout));
	globalTpl->Set(String::New("setInterval"), FunctionTemplate::New(setInterval));
	globalTpl->Set(String::New("clearInterval"), FunctionTemplate::New(clearInterval));

	globalTpl->Set(String::New("Image"), CImage::exportJS());
	globalTpl->Set(String::New("CanvasRenderingContext2D"), CCanvasContext::exportJS());
	globalTpl->Set(String::New("XMLHttpRequest"), CXMLHttpRequest::exportJS());
	globalTpl->Set(String::New("Socket"), CSocket::exportJS());

	m_ctx = Context::New(0, globalTpl);

	Context::Scope contextScope(m_ctx);

	// 添加全局canvas变量
	m_ctx->Global()->Set(String::New("canvas"), CCanvas::getJSObject());
	
	do {
		string indexPath = m_path + "index.js";
		string indexContent;

		int status = StringUtil::loadContentsOfURL( indexPath, indexContent );
		LOG("index.js: %s", indexContent.c_str());

		char *pch = (char *)indexContent.c_str();
		char *newPch = pch;
		int len = 0;
		bool jsRunSuccess = true;

		TryCatch tryCatch;

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

			Handle<String> source = String::New(jsContent.c_str());
			Handle<Script> script = Script::Compile(source);
			if( script.IsEmpty() )
			{
				String::Utf8Value error(tryCatch.Exception());
				LOG("js error:%s", *error);
				jsRunSuccess = false;
				break;
			}

			Handle<Value> result = script->Run();
			if( result.IsEmpty() )
			{
				String::Utf8Value error(tryCatch.Exception());
				LOG("js error:%s", *error);
				jsRunSuccess = false;
				break;
			}
			else
			{
				LOG("init run js success");
			}
		}

		if( !jsRunSuccess )
		{
			break;
		}

		
		Handle<Value> jsReadyValue = m_ctx->Global()->Get(String::New("documentReady"));
		if( jsReadyValue.IsEmpty() || !jsReadyValue->IsFunction() )
		{
			LOG("no documentReady func");
			break;
		}

		if( !callJSFunction(Handle<Function>::Cast(jsReadyValue->ToObject()), 0, 0) )
		{
			break;
		}

		return true;
	}while(false);

	return false;
}

bool CV8Context::callJSFunction(Handle<Function> func, int argc, Handle<Value> argv[])
{
	HandleScope handleScope;

	Context::Scope contextScope(m_ctx);

	TryCatch tryCatch;

	Handle<Value> result = func->Call(m_ctx->Global(), argc, argv);
	if( result.IsEmpty() )
	{
		String::Utf8Value error(tryCatch.Exception());
		LOG("js error:%s", *error);
		return false;
	}
	
	return true;
}

const string & CV8Context::path()
{
	return m_path;
}

const Persistent<Context> CV8Context::context()
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
