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
#include "image.h"


class CCanvasContext
{
	private:
		float m_r,m_g,m_b;
		string m_fillStyle;
	
	public:
		float globalAlpha;
	
	public:
		JS_CLASS_EXPORT_DEF(CCanvasContext)

		void set_fillStyle(string fillStyle);	
		string & get_fillStyle();

		CCanvasContext() { m_r = m_g = m_b = 0.0; globalAlpha = 1.0; }

		void drawImage(CImage *image, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh);
		void drawImageBatch(CImage *image, int count, float *coords);

		void clearRect(float x, float y, float width, float height);
		void fillRect(float x, float y, float width, float height);

		void save();
		void restore();

		void translate(float x, float y);
		void scale(float scaleX, float scaleY);
		void rotate(float angle);

		void setTransform(float m11, float m12, float m21, float m22, float dx, float dy);
};

