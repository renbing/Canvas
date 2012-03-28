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
#include "label.h"

#include <vector>
using std::vector;

typedef struct
{
	float r;
	float g;
	float b;
	float a;
}Color4f;

class CCanvasContext
{
	private:
		Color4f m_fillColor;
		string m_fillStyle;

		Color4f m_strokeColor;
		string m_strokeStyle;
	
	public:
		float globalAlpha;
	
	public:
		JS_CLASS_EXPORT_DEF(CCanvasContext)

		void set_fillStyle(string fillStyle);	
		const string & get_fillStyle() { return m_fillStyle; };

		void set_strokeStyle(string strokeStyle);	
		const string & get_strokeStyle() { return m_strokeStyle; };

		CCanvasContext();

		void drawImage(CImage *image, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh);
		void drawImageBatch(CImage *image, int count, float *coords);

		void drawLabel(CLabel *label, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh);

		void clearRect(float x, float y, float width, float height);
		void fillRect(float x, float y, float width, float height);

		void save();
		void restore();

		void translate(float x, float y);
		void scale(float scaleX, float scaleY);
		void rotate(float angle);

		void setTransform(float m11, float m12, float m21, float m22, float dx, float dy);
	
	// 绘图缓冲优化
	private:
		CImage *m_image;	//上次绘图纹理
		vector<float> m_coords;
	public:
		void drawImageBatch(CImage *image, const vector<float> &coords);
		void cleanDrawImage();
};

