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
#include "bitmap.h"

#include <vector>
using std::vector;

typedef struct
{
	float r;
	float g;
	float b;
	float a;
}Color4f;

typedef struct
{
    float x;
    float y;
}CGPoint;

static inline CGPoint CGPointMake(float x, float y)
{
    CGPoint point = {x, y};
    return point;
}

typedef enum {BRUSH_UNFOCUS,BRUSH_FOCUSED,BRUSH_MOVING} BrushState;

class CCanvasContext
{
	private:
		Color4f m_fillStyleColor;
		string m_fillStyle;

		Color4f m_strokeStyleColor;
		string m_strokeStyle;

        CGPoint m_brushCursor;
        BrushState m_brushState;
        vector< vector<CGPoint> > m_lines;
	
	public:
		float globalAlpha;

        float lineWidth;
        float miterLimit;
        string lineCap;
        string lineJoin;
	
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
		void drawBitmap(CBitmap *bitmap, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh);

		void clearRect(float x, float y, float width, float height);
		void fillRect(float x, float y, float width, float height);
        
        // 矩阵,状态机相关
		void save();
		void restore();

		void translate(float x, float y);
		void scale(float scaleX, float scaleY);
		void rotate(float angle);

		void setTransform(float m11, float m12, float m21, float m22, float dx, float dy);

        //画图相关
        void beginPath();
        void closePath();
        void moveTo(float x, float y);
        void lineTo(float x, float y);
        void stroke();
	
	// 绘图缓冲优化
	private:
		CImage *m_image;	//上次绘图纹理
		vector<float> m_coords;
	public:
		void drawImageBatch(CImage *image, const vector<float> &coords);
		void cleanDrawImage();
};

