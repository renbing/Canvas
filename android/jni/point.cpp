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

#include "point.h"

JS_PROPERTY_BYFUNC(Point, Int32, x)
JS_PROPERTY(Point, Int32, y)
JS_PROPERTY_BYFUNC(Point, String, src)

JS_SIMPLE_FUNCTION(Point, talk)

static JSStaticValue jsStaticValues[] = {
	JS_PROPERTY_DEF(x),
	JS_PROPERTY_DEF(y),
	JS_PROPERTY_DEF(src),
	{0, 0, 0}
};

static JSStaticFunction jsStaticFunctions[] = {
	JS_FUNCTION_DEF(talk),
	{0, 0}
};

JS_CLASS_EXPORT(Point,"Point")


void Point::talk()
{
	LOG("point talk: x=%d y=%d", x, y);
}

void Point::set_src(string s)
{
	LOG("point set src");
	src = s;
}
