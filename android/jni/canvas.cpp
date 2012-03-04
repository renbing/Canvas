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

static Handle<Value> JS_addEventListener( const Arguments& args )
{
	HandleScope handleScope;

	int argc = args.Length();

	Local<Object> self = args.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	CCanvas *canvas = static_cast<CCanvas *>(wrap->Value());

	if( !canvas || argc < 2 || !args[0]->IsString() || !args[1]->IsFunction() )
	{
		return Undefined();
	}

	string eventName;
	eventName = ValueCast::Cast(eventName, args[0]);

	Persistent<Function> callback = Persistent<Function>::New( Local<Function>::Cast(args[1]) );

	canvas->addEventListener(eventName, callback);

	return Undefined();
}

static Handle<Value> JS_getContext( const Arguments& args )
{
	HandleScope handleScope;

	int argc = args.Length();

	Local<Object> self = args.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	CCanvas *canvas = static_cast<CCanvas *>(wrap->Value());

	if( !canvas )
	{
		return Undefined();
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

void CCanvas::addEventListener(string &eventName, Persistent<Function> callback, bool capture)
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
	Locker locker;

	HandleScope handleScope;

	Context::Scope contextScope(CV8Context::getInstance()->context());

	Persistent<Function> callback;

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

	Handle<Object> eventObj = Object::New();
	eventObj->Set(String::New("pageX"), Number::New(x), ReadOnly);
	eventObj->Set(String::New("pageY"), Number::New(y), ReadOnly);
	eventObj->Set(String::New("clientX"), Number::New(x), ReadOnly);
	eventObj->Set(String::New("clientY"), Number::New(y), ReadOnly);
	eventObj->Set(String::New("offsetX"), Number::New(x), ReadOnly);
	eventObj->Set(String::New("offsetY"), Number::New(y), ReadOnly);
	eventObj->Set(String::New("layerX"), Number::New(x), ReadOnly);
	eventObj->Set(String::New("layerY"), Number::New(y), ReadOnly);

	Handle<Value> argv[] = {eventObj};

	CV8Context::getInstance()->callJSFunction(callback, 1, argv);
}
