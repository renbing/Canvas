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
	public:
		int width;
		int height;
		unsigned long POTWidth;
		unsigned long POTHeight;
		bool hasAlpha;

		GLuint texture;

		v8::Persistent<v8::Function> onload;

	private:
		string m_src;
	
	public:
		JS_CLASS_EXPORT_DEF(CImage)
		
		CImage();
		~CImage();

		void set_src(string src);
		string & get_src();

		void createTextureWithPngData(PNGSOURCE source, const void *input, png_rw_ptr read_fn);
};
