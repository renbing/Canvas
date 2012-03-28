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

#include "canvas.h"
#include "js.h"

static v8::Handle<v8::Value> JS_addEventListener( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	CCanvas *canvas = jsGetCObject<CCanvas>(args.Holder());

	if( !canvas || argc < 2 || !args[0]->IsString() || !args[1]->IsFunction() )
	{
		return v8::Undefined();
	}

	string eventName;
	eventName = ValueCast::Cast(eventName, args[0]);

	v8::Persistent<v8::Function> callback = v8::Persistent<v8::Function>::New( v8::Local<v8::Function>::Cast(args[1]) );

	canvas->addEventListener(eventName, callback);

	return v8::Undefined();
}

static v8::Handle<v8::Value> JS_getContext( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	CCanvas *canvas = jsGetCObject<CCanvas>(args.Holder());

	if( !canvas )
	{
		return v8::Undefined();
	}

	CCanvasContext *context = canvas->getContext2D();
		
	// 返回一个CCanvasContext的JS对象
	return CCanvasContext::newJSObject(context);
}

JS_PROPERTY_READONLY(CCanvas, Int32, width)
JS_PROPERTY_READONLY(CCanvas, Int32, height)

static JSStaticValue jsStaticValues[] = {
	JS_PROPERTY_READONLY_DEF(width),
	JS_PROPERTY_READONLY_DEF(height),
	{0, 0, 0}
};

static JSStaticFunction jsStaticFunctions[] = {
	JS_FUNCTION_DEF(addEventListener),
	JS_FUNCTION_DEF(getContext),
	{0, 0}
};

JS_OBJECT_EXPORT(CCanvas)

CCanvas *CCanvas::m_instance = NULL;

CCanvas::CCanvas()
{
	width = 0;
	height = 0;

	m_context = new CCanvasContext();
}

CCanvas::~CCanvas()
{
	delete m_context;
}

CCanvas * CCanvas::getInstance()
{
	if( m_instance == NULL )
	{
		// 需要加锁在这个位置
		if( m_instance == NULL )
		{
			m_instance = new CCanvas();
		}
	}

	return m_instance;
}

void CCanvas::addEventListener(string &eventName, v8::Persistent<v8::Function> callback, bool capture)
{
	if( eventName == "mousedown" )
	{
		m_eventDown.Dispose();
		m_eventDown = callback;
	}
	else if( eventName == "mousemove" )
	{
		m_eventMove.Dispose();
		m_eventMove = callback;
	}
	else if( eventName == "mouseup" )
	{
		m_eventUp.Dispose();
		m_eventUp = callback;
	}
}

CCanvasContext * CCanvas::getContext2D()
{
	return m_context;
}

void CCanvas::onTouch( TouchAction e, float x, float y)
{
	v8::Locker locker;

	v8::HandleScope handleScope;

	v8::Context::Scope contextScope(CV8Context::getInstance()->context());

	v8::Persistent<v8::Function> callback;

	switch( e )
	{
		case TOUCH_DOWN:
			callback = m_eventDown;
			break;
		case TOUCH_MOVE:
			callback = m_eventMove;
			break;
		case TOUCH_UP:
			callback = m_eventUp;
			break;
		default:
			break;
	}

	if( callback.IsEmpty() )
	{
		return;
	}

	v8::Handle<v8::Object> eventObj = v8::Object::New();
	eventObj->Set(v8::String::New("pageX"), v8::Number::New(x), v8::ReadOnly);
	eventObj->Set(v8::String::New("pageY"), v8::Number::New(y), v8::ReadOnly);
	eventObj->Set(v8::String::New("clientX"), v8::Number::New(x), v8::ReadOnly);
	eventObj->Set(v8::String::New("clientY"), v8::Number::New(y), v8::ReadOnly);
	eventObj->Set(v8::String::New("offsetX"), v8::Number::New(x), v8::ReadOnly);
	eventObj->Set(v8::String::New("offsetY"), v8::Number::New(y), v8::ReadOnly);
	eventObj->Set(v8::String::New("layerX"), v8::Number::New(x), v8::ReadOnly);
	eventObj->Set(v8::String::New("layerY"), v8::Number::New(y), v8::ReadOnly);

	v8::Handle<v8::Value> argv[] = {eventObj};

	CV8Context::getInstance()->callJSFunction(callback, 1, argv);
}
