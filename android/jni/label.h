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

#include <string>

using std::string;

class CLabel
{
	private:
		GLuint m_texture;

		unsigned int m_fillColor;
		unsigned int m_strokeColor;

		bool m_fontBold;
		bool m_fontItalic;

		float m_fontSize;
		string m_fontSizeUnit;

		string m_fontName;

		/*	[font style] [font weight] [font size] [font name]
			style: normal, italic, oblique, inherit		
			weight: normal/lighter/<600, bold/bolder/>=600
			size: 20px 20pt 20mm 2cm 1in
		*/
		string m_font;

		// #ffffff,#fff,white,rgb(255,255,255)
		string m_fillStyle;
		string m_strokeStyle;
		
		unsigned long m_POTWidth;
		unsigned long m_POTHeight;

	public:
		CLabel();
		~CLabel();

		JS_CLASS_EXPORT_DEF(CLabel)

		void set_font(string font);
		const string & get_font() { return m_font; };

		void set_fillStyle(string fillStyle);
		const string & get_fillStyle() { return m_fillStyle; };

		void set_strokeStyle(string strokeStyle);
		const string & get_strokeStyle() { return m_strokeStyle; };
		
		unsigned int width;
		unsigned int height;

		// start, end, left,right, center。默认值:start  start=left end=right
		string textAlign;

		// 暂不支持 top, hanging, middle,alphabetic, ideographic, bottom。默认值：alphabetic
		//string textBaseline;

		string text;

		unsigned long POTWidth() { return m_POTWidth; }
		unsigned long POTHeight() { return m_POTHeight; }

		bool needUpdate;

		GLuint getTexture();
};
