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

#include "texture.h"

TextureManager * getInstance() 
{
	if( m_instance == NULL )
	{
		if( m_instance == NULL )
		{
			m_instance = new TextureManager();
		}
	}

	return m_instance;
}

GLuint TextureManager::genTexture(const string &key, void *pixels)
{
}

GLuint TextureManager::getTexture(const string &key)
{
}

void TextureManager::deleteTexture(const string &key)
{
}

void TextureManager::clean()
{
}
