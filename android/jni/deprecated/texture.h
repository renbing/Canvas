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

#include <map>
using std::map;

class TextureManager
{
	private:
		static TextureManager *m_instance;

		map<string, GLuint> m_textures;
	
	private:
		TextureManager();
	
	public:
		~TextureManager();

		static TextureManager *getInstance();

		GLuint genTexture(const string &key, void *pixels);
		GLuint getTexture(const string &key);
		void deleteTexture(const string &key);
		void clean();
};
