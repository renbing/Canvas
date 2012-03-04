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

class Point
{
	public:
		Point(int x, int y) : x(x), y(y) {}
		Point(){ x=y=0;}
		~Point(){ };
		int x,y;

		
		void talk();
		void set_src(string src);
		string& get_src();
		void set_x(int _x){LOG("set x:%d", _x); x = _x;};
		int & get_x(){return x;};

		string src;

	public:
		JS_CLASS_EXPORT_DEF(Point)
};
