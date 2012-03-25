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

#include "global.h"
#include "context.h"

typedef enum {TOUCH_DOWN, TOUCH_MOVE, TOUCH_UP} TouchAction;

class CCanvas
{
	private:
		CCanvasContext *m_context;

		v8::Persistent<v8::Function> m_eventDown;
		v8::Persistent<v8::Function> m_eventUp;
		v8::Persistent<v8::Function> m_eventMove;

		static CCanvas *m_instance;

	public:
		float width;
		float height;
	
	private:
		CCanvas();
	
	public:
		JS_OBJECT_EXPORT_DEF
		
		~CCanvas();
		static CCanvas * getInstance();

		void addEventListener(string &eventName, v8::Persistent<v8::Function> callback, bool capture = true);
		CCanvasContext * getContext2D();
		void onTouch( TouchAction e, float x, float y);
};
