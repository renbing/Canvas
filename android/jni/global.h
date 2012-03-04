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

using namespace v8;
using namespace std;


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
	AccessorGetter getter;
	AccessorSetter setter;
} JSStaticValue;

typedef struct {
	const char * const name;
	InvocationCallback callback;
} JSStaticFunction;


class ValueCast
{
	public:
		static bool Cast(bool &, Handle<Value> value) { return value->BooleanValue(); }
		static float Cast(float &, Handle<Value> value) { return value->NumberValue(); }
		static int Cast(int &, Handle<Value> value) { return value->Int32Value(); }
		static unsigned int Cast(unsigned int &, Handle<Value> value) { return value->Uint32Value(); }
		static unsigned long Cast(unsigned long &, Handle<Value> value) { return value->Uint32Value(); }
		static string Cast(string &, Handle<Value> value) { return string(*(String::Utf8Value(value))); }
		static Persistent<Function> Cast(Handle<Function>, Handle<Value> value) { return Persistent<Function>::New( Handle<Function>::Cast(value) ); };

		static Handle<Value> New(bool &value) { return Boolean::New(value); }
		static Handle<Value> New(float &value) { return Number::New(value); }
		static Handle<Value> New(int &value) { return Integer::New(value); }
		static Handle<Value> New(unsigned int &value) { return Integer::NewFromUnsigned(value); }
		static Handle<Value> New(unsigned long &value) { return Integer::NewFromUnsigned(value); }
		static Handle<Value> New(string &value) { return String::New(value.c_str()); }
		static Handle<Value> New(Handle<Function> &value) { return value; }
};

static Handle<FunctionTemplate> createJSFunctionTemplate( InvocationCallback jsConstructor, JSStaticValue jsStaticValues[], int propertyCount, JSStaticFunction jsStaticFunctions[], int funcCount )
{
	HandleScope handleScope;

	Handle<FunctionTemplate> funcTpl = FunctionTemplate::New(jsConstructor);
	Handle<ObjectTemplate> funcProto = funcTpl->PrototypeTemplate();
	
	for( int i=0; i<funcCount; i++ )
	{
		JSStaticFunction &define = jsStaticFunctions[i];
		if( !define.name )
		{
			break;
		}
		funcProto->Set(String::New(define.name), FunctionTemplate::New(define.callback));
	}

	Handle<ObjectTemplate> funcIns = funcTpl->InstanceTemplate();
	funcIns->SetInternalFieldCount(1);

	for( int i=0; i<propertyCount; i++ )
	{
		JSStaticValue &define = jsStaticValues[i];
		if( !define.name )
		{
			break;
		}
		funcIns->SetAccessor(String::New(define.name), define.getter, define.setter);
	}

	return handleScope.Close(funcTpl);
}

static Handle<ObjectTemplate> createJSObjectTemplate(JSStaticValue jsStaticValues[], int propertyCount, JSStaticFunction jsStaticFunctions[], int funcCount )
{
	HandleScope handleScope;

	Handle<ObjectTemplate> objTpl = ObjectTemplate::New();
	objTpl->SetInternalFieldCount(1);

	for( int i=0; i<funcCount; i++ )
	{
		JSStaticFunction &define = jsStaticFunctions[i];
		if( !define.name )
		{
			break;
		}
		objTpl->Set(String::New(define.name), FunctionTemplate::New(define.callback));
	}

	for( int i=0; i<propertyCount; i++ )
	{
		JSStaticValue &define = jsStaticValues[i];
		if( !define.name )
		{
			break;
		}
		objTpl->SetAccessor(String::New(define.name), define.getter, define.setter);
	}

	return handleScope.Close(objTpl);
}

template<class T>
void jsDestructor(Persistent<Value> self, void *parameter)
{
	LOG("jsDestructor called");
	delete static_cast<T *>(parameter);
	self.ClearWeak();
}

template<class T>
inline Handle<Value> jsConstructor( const Arguments& args )
{
	HandleScope handleScope;

	if( args.IsConstructCall() )
	{
		T *obj = new T;

		Persistent<Object> self = Persistent<Object>::New(args.Holder());
		self.MakeWeak(obj, jsDestructor<T>);
		self->SetInternalField(0, External::New(obj));

		return handleScope.Close(self);
	}

	return Undefined();
}

template<class T>
inline T * getCObjct(Handle<Object> obj)
{
	if( obj.IsEmpty() )
	{
		return NULL;
	}

	Local<External> wrap = Local<External>::Cast(obj->GetInternalField(0));
	if( wrap.IsEmpty() )
	{
		return NULL;
	}

	T *cobj = static_cast<T *>(wrap->Value());
	
	return cobj;
}

#define JS_PROPERTY_READONLY(className, varType, varName)\
static Handle<Value> JS_get##varName(Local<String> property, const AccessorInfo &info) \
{\
	HandleScope handleScope;\
\
	Local<Object> self = info.Holder();\
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));\
\
	return handleScope.Close(ValueCast::New(static_cast<className *>(wrap->Value())->varName));\
}

#define JS_PROPERTY_WRITE(className, varType, varName)\
static void JS_set##varName(Local<String> property, Local<Value> value, const AccessorInfo &info)\
{\
	HandleScope handleScope;\
\
	Local<Object> self = info.Holder();\
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));\
\
	className *obj = static_cast<className *>(wrap->Value());\
	obj->varName = ValueCast::Cast(obj->varName, value);\
}

#define JS_PROPERTY_READONLY_BYFUNC(className, varType, varName)\
static Handle<Value> JS_get##varName(Local<String> property, const AccessorInfo &info) \
{\
	HandleScope handleScope;\
\
	Local<Object> self = info.Holder();\
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));\
\
	return handleScope.Close(ValueCast::New(static_cast<className *>(wrap->Value())->get_##varName()));\
}

#define JS_PROPERTY_WRITE_BYFUNC(className, varType, varName)\
static void JS_set##varName(Local<String> property, Local<Value> value, const AccessorInfo &info)\
{\
	HandleScope handleScope;\
\
	Local<Object> self = info.Holder();\
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));\
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
static Handle<Value> JS_##funcName( const Arguments& args )\
{\
	HandleScope handleScope;\
\
	Local<Object> self = args.Holder();\
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));\
\
	static_cast<className *>(wrap->Value())->funcName();\
\
	return Undefined();\
}

#define JS_PROPERTY_READONLY_DEF(varName)\
{#varName, JS_get##varName, NULL}

#define JS_PROPERTY_DEF(varName)\
{#varName, JS_get##varName, JS_set##varName}

#define JS_FUNCTION_DEF(funcName)\
{#funcName, JS_##funcName}

#define JS_CLASS_EXPORT_DEF(className) \
static Handle<FunctionTemplate> exportJS();\
static Handle<Object> newJSObject(className *cobj);

#define JS_CLASS_EXPORT(className, JSClassName)\
Handle<FunctionTemplate> className::exportJS()\
{\
	HandleScope handleScope;\
\
	int funcCount = sizeof(jsStaticFunctions)/sizeof(JSStaticFunction);\
	int propertyCount = sizeof(jsStaticValues)/sizeof(JSStaticValue);\
	LOG("exportJS: %s funcCount=%d propertyCount=%d", #className, funcCount, propertyCount);\
\
	return handleScope.Close(createJSFunctionTemplate(jsConstructor<className>, jsStaticValues, propertyCount, jsStaticFunctions, funcCount));\
}\
Handle<Object> className::newJSObject(className *cobj)\
{\
	HandleScope handleScope;\
\
	Handle<FunctionTemplate> tpl = className::exportJS();\
	Local<Object> obj = tpl->GetFunction()->NewInstance();\
	obj->SetInternalField(0, External::New(cobj));\
\
	return handleScope.Close(obj);\
}

#define JS_OBJECT_EXPORT_DEF \
static Handle<Object> getJSObject();

#define JS_OBJECT_EXPORT(className)\
Handle<Object> className::getJSObject()\
{\
	HandleScope handleScope;\
\
	int funcCount = sizeof(jsStaticFunctions)/sizeof(JSStaticFunction);\
	int propertyCount = sizeof(jsStaticValues)/sizeof(JSStaticValue);\
	LOG("exportObject: %s funcCount=%d propertyCount=%d", #className, funcCount, propertyCount);\
\
	Local<Object> obj = createJSObjectTemplate(jsStaticValues, propertyCount, jsStaticFunctions, funcCount)->NewInstance();\
	obj->SetInternalField(0, External::New(className::getInstance()));\
\
	return handleScope.Close(obj);\
}
