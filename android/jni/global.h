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

#include <android/log.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "ProjectName", __VA_ARGS__)

#include <v8.h>
#include <zip.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>

using std::string;


static unsigned long computePOT(unsigned long x)
{
	x = x - 1;
	x = x | (x >> 1);
	x = x | (x >> 2);
	x = x | (x >> 4);
	x = x | (x >> 8);
	x = x | (x >>16);
	return x + 1;
}

static void mysleep(int msec)
{
	struct timeval delay;
	delay.tv_sec = (int) msec/1000;
	delay.tv_usec = (msec % 1000) * 1000;
	select(0, NULL, NULL, NULL, &delay);
}


// JS 处理相关
typedef struct {
	const char * const name;
	v8::AccessorGetter getter;
	v8::AccessorSetter setter;
} JSStaticValue;

typedef struct {
	const char * const name;
	v8::InvocationCallback callback;
} JSStaticFunction;


class ValueCast
{
	public:
		static bool Cast(bool &, v8::Handle<v8::Value> value) { return value->BooleanValue(); }
		static float Cast(float &, v8::Handle<v8::Value> value) { return value->NumberValue(); }
		static int Cast(int &, v8::Handle<v8::Value> value) { return value->Int32Value(); }
		static unsigned int Cast(unsigned int &, v8::Handle<v8::Value> value) { return value->Uint32Value(); }
		static unsigned long Cast(unsigned long &, v8::Handle<v8::Value> value) { return value->Uint32Value(); }
		static string Cast(string &, v8::Handle<v8::Value> value) { return string(*(v8::String::Utf8Value(value))); }
		static v8::Persistent<v8::Function> Cast(v8::Handle<v8::Function>, v8::Handle<v8::Value> value) 
		{ 
			return v8::Persistent<v8::Function>::New( v8::Handle<v8::Function>::Cast(value) ); 
		};

		static v8::Handle<v8::Value> New(bool &value) { return v8::Boolean::New(value); }
		static v8::Handle<v8::Value> New(float &value) { return v8::Number::New(value); }
		static v8::Handle<v8::Value> New(int &value) { return v8::Integer::New(value); }
		static v8::Handle<v8::Value> New(unsigned int &value) { return v8::Integer::NewFromUnsigned(value); }
		static v8::Handle<v8::Value> New(unsigned long &value) { return v8::Integer::NewFromUnsigned(value); }
		static v8::Handle<v8::Value> New(string &value) { return v8::String::New(value.c_str()); }
		static v8::Handle<v8::Value> New(v8::Handle<v8::Function> &value) { return value; }
};

static v8::Handle<v8::FunctionTemplate> createJSFunctionTemplate( v8::InvocationCallback jsConstructor, 
																JSStaticValue jsStaticValues[], 
																int propertyCount, 
																JSStaticFunction jsStaticFunctions[], 
																int funcCount )
{
	v8::HandleScope handleScope;

	v8::Handle<v8::FunctionTemplate> funcTpl = v8::FunctionTemplate::New(jsConstructor);
	v8::Handle<v8::ObjectTemplate> funcProto = funcTpl->PrototypeTemplate();
	
	for( int i=0; i<funcCount; i++ )
	{
		JSStaticFunction &define = jsStaticFunctions[i];
		if( !define.name )
		{
			break;
		}
		funcProto->Set(v8::String::New(define.name), v8::FunctionTemplate::New(define.callback));
	}

	v8::Handle<v8::ObjectTemplate> funcIns = funcTpl->InstanceTemplate();
	funcIns->SetInternalFieldCount(1);

	for( int i=0; i<propertyCount; i++ )
	{
		JSStaticValue &define = jsStaticValues[i];
		if( !define.name )
		{
			break;
		}
		funcIns->SetAccessor(v8::String::New(define.name), define.getter, define.setter);
	}

	return handleScope.Close(funcTpl);
}

static v8::Handle<v8::ObjectTemplate> createJSObjectTemplate(JSStaticValue jsStaticValues[], 
															int propertyCount, 
															JSStaticFunction jsStaticFunctions[], 
															int funcCount )
{
	v8::HandleScope handleScope;

	v8::Handle<v8::ObjectTemplate> objTpl = v8::ObjectTemplate::New();
	objTpl->SetInternalFieldCount(1);

	for( int i=0; i<funcCount; i++ )
	{
		JSStaticFunction &define = jsStaticFunctions[i];
		if( !define.name )
		{
			break;
		}
		objTpl->Set(v8::String::New(define.name), v8::FunctionTemplate::New(define.callback));
	}

	for( int i=0; i<propertyCount; i++ )
	{
		JSStaticValue &define = jsStaticValues[i];
		if( !define.name )
		{
			break;
		}
		objTpl->SetAccessor(v8::String::New(define.name), define.getter, define.setter);
	}

	return handleScope.Close(objTpl);
}

template<class T>
void jsDestructor(v8::Persistent<v8::Value> self, void *parameter)
{
	LOG("jsDestructor called");
	delete static_cast<T *>(parameter);
	self.ClearWeak();
}

template<class T>
inline v8::Handle<v8::Value> jsConstructor( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	if( args.IsConstructCall() )
	{
		T *obj = new T;

		v8::Persistent<v8::Object> self = v8::Persistent<v8::Object>::New(args.Holder());
		self.MakeWeak(obj, jsDestructor<T>);
		self->SetInternalField(0, v8::External::New(obj));

		return handleScope.Close(self);
	}

	return v8::Undefined();
}

template<class T>
inline T * getCObjct( v8::Handle<v8::Object> obj )
{
	if( obj.IsEmpty() )
	{
		return NULL;
	}

	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(obj->GetInternalField(0));
	if( wrap.IsEmpty() )
	{
		return NULL;
	}

	T *cobj = static_cast<T *>(wrap->Value());
	
	return cobj;
}

#define JS_PROPERTY_READONLY(className, varType, varName)\
static v8::Handle<v8::Value> JS_get##varName( v8::Local<v8::String> property, const v8::AccessorInfo &info ) \
{\
	v8::HandleScope handleScope;\
\
	v8::Local<v8::Object> self = info.Holder();\
	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));\
\
	return handleScope.Close(ValueCast::New(static_cast<className *>(wrap->Value())->varName));\
}

#define JS_PROPERTY_WRITE(className, varType, varName)\
static void JS_set##varName( v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo &info )\
{\
	v8::HandleScope handleScope;\
\
	v8::Local<v8::Object> self = info.Holder();\
	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));\
\
	className *obj = static_cast<className *>(wrap->Value());\
	obj->varName = ValueCast::Cast(obj->varName, value);\
}

#define JS_PROPERTY_READONLY_BYFUNC(className, varType, varName)\
static v8::Handle<v8::Value> JS_get##varName( v8::Local<v8::String> property, const v8::AccessorInfo &info ) \
{\
	v8::HandleScope handleScope;\
\
	v8::Local<v8::Object> self = info.Holder();\
	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));\
\
	return handleScope.Close(ValueCast::New(static_cast<className *>(wrap->Value())->get_##varName()));\
}

#define JS_PROPERTY_WRITE_BYFUNC(className, varType, varName)\
static void JS_set##varName( v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo &info )\
{\
	v8::HandleScope handleScope;\
\
	v8::Local<v8::Object> self = info.Holder();\
	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));\
\
	className *obj = static_cast<className *>(wrap->Value());\
	obj->set_##varName( ValueCast::Cast(obj->get_##varName(), value) );\
}



#define JS_PROPERTY(className, varType, varName)\
JS_PROPERTY_READONLY(className, varType, varName)\
JS_PROPERTY_WRITE(className, varType, varName)

#define JS_PROPERTY_BYFUNC(className, varType, varName)\
JS_PROPERTY_READONLY_BYFUNC(className, varType, varName)\
JS_PROPERTY_WRITE_BYFUNC(className, varType, varName)\

#define JS_SIMPLE_FUNCTION(className, funcName)\
static v8::Handle<v8::Value> JS_##funcName( const v8::Arguments& args )\
{\
	v8::HandleScope handleScope;\
\
	v8::Local<v8::Object> self = args.Holder();\
	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));\
\
	static_cast<className *>(wrap->Value())->funcName();\
\
	return v8::Undefined();\
}

#define JS_PROPERTY_READONLY_DEF(varName)\
{#varName, JS_get##varName, NULL}

#define JS_PROPERTY_DEF(varName)\
{#varName, JS_get##varName, JS_set##varName}

#define JS_FUNCTION_DEF(funcName)\
{#funcName, JS_##funcName}

#define JS_CLASS_EXPORT_DEF(className) \
static v8::Handle<v8::FunctionTemplate> exportJS();\
static v8::Handle<v8::Object> newJSObject(className *cobj);

#define JS_CLASS_EXPORT(className, JSClassName)\
v8::Handle<v8::FunctionTemplate> className::exportJS()\
{\
	v8::HandleScope handleScope;\
\
	int funcCount = sizeof(jsStaticFunctions)/sizeof(JSStaticFunction);\
	int propertyCount = sizeof(jsStaticValues)/sizeof(JSStaticValue);\
	LOG("exportJS: %s funcCount=%d propertyCount=%d", #className, funcCount, propertyCount);\
\
	return handleScope.Close(createJSFunctionTemplate(jsConstructor<className>, jsStaticValues, propertyCount, jsStaticFunctions, funcCount));\
}\
\
v8::Handle<v8::Object> className::newJSObject(className *cobj)\
{\
	v8::HandleScope handleScope;\
\
	v8::Handle<v8::FunctionTemplate> tpl = className::exportJS();\
	v8::Local<v8::Object> obj = tpl->GetFunction()->NewInstance();\
	obj->SetInternalField(0, v8::External::New(cobj));\
\
	return handleScope.Close(obj);\
}

#define JS_OBJECT_EXPORT_DEF \
static v8::Handle<v8::Object> getJSObject();

#define JS_OBJECT_EXPORT(className)\
v8::Handle<v8::Object> className::getJSObject()\
{\
	v8::HandleScope handleScope;\
\
	int funcCount = sizeof(jsStaticFunctions)/sizeof(JSStaticFunction);\
	int propertyCount = sizeof(jsStaticValues)/sizeof(JSStaticValue);\
	LOG("exportObject: %s funcCount=%d propertyCount=%d", #className, funcCount, propertyCount);\
\
	v8::Local<v8::Object> obj = createJSObjectTemplate(jsStaticValues, propertyCount, jsStaticFunctions, funcCount)->NewInstance();\
	obj->SetInternalField(0, v8::External::New(className::getInstance()));\
\
	return handleScope.Close(obj);\
}
