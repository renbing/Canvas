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

		Persistent<Function> m_eventDown;
		Persistent<Function> m_eventUp;
		Persistent<Function> m_eventMove;

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

		void addEventListener(string &eventName, Persistent<Function> callback, bool capture = true);
		CCanvasContext * getContext2D();
		void onTouch( TouchAction e, float x, float y);
};
