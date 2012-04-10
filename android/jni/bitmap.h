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

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <jni.h>

#include <string>

using std::string;

typedef struct {
	bool bold;
	bool italic;
	float size;
	string unit;
	string name;
}CFont;

typedef struct {
	int x;
	int y;
	int width;
	int height;
}CRect;

class CBitmap
{
	private:
		GLuint m_texture;

		CFont m_cfont;
		unsigned int m_fillColor;
		unsigned int m_strokeColor;

		string m_font;
		string m_fillStyle;
		string m_strokeStyle;
		
		unsigned long m_width;
		unsigned long m_height;
		unsigned long m_POTWidth;
		unsigned long m_POTHeight;

		jobject m_jbitmap;
		unsigned char *m_buf;

	public:
		string textAlign;
		float lineWidth;
		float miterLimit;
		string lineCap;
		string lineJoin;
	
	private:
		void init();
		void updateTexture(jobject jrect = NULL);

	public:
		CBitmap();
		CBitmap(unsigned long w, unsigned long h);
		~CBitmap();

		JS_CLASS_EXPORT_DEF(CBitmap)

		void set_font(string font);
		const string & get_font() { return m_font; };

		void set_fillStyle(string fillStyle);
		const string & get_fillStyle() { return m_fillStyle; };

		void set_strokeStyle(string strokeStyle);
		const string & get_strokeStyle() { return m_strokeStyle; };
		
		unsigned long get_width() { return m_width; }
		unsigned long get_height() { return m_height; }

		unsigned long POTWidth() { return m_POTWidth; }
		unsigned long POTHeight() { return m_POTHeight; }

		GLuint getTexture() { return m_texture; };

		void beginPath();
		void closePath();
		void stroke();
		void moveTo(float x, float y);
		void lineTo(float x, float y);
		void clearRect(float x, float y, float w, float h);
		void fillText(const string &text, float x, float y, float maxWidth);
};
