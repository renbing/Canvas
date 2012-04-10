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
#include <png.h>

typedef enum {ZIP, RAW} PNGSOURCE;

class CImage
{
	private:
		unsigned long m_width;
		unsigned long m_height;
		unsigned long m_POTWidth;
		unsigned long m_POTHeight;
		bool m_hasAlpha;

		GLuint m_texture;

		string m_src;

	public:
		v8::Persistent<v8::Function> onload;
	
	public:
		JS_CLASS_EXPORT_DEF(CImage)
		
		CImage();
		~CImage();

		unsigned long get_width() { return m_width; }		
		unsigned long get_height() { return m_height; }

		void set_src(string src);
		const string & get_src() { return m_src; };

		unsigned int POTWidth() { return m_POTWidth; }
		unsigned int POTHeight() { return m_POTHeight; }
		bool hasAlpha() { return m_hasAlpha; }
		GLuint getTexture() { return m_texture; }

		void createTextureWithPngData(PNGSOURCE source, const void *input, png_rw_ptr read_fn);
};
