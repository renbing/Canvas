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


#include "global.h"
#include "stringutil.h"
#include "jnihelper.h"

#include "label.h"

extern JavaVM *g_jvm;
extern jobject g_jgl;

template<>
inline void jsSetPropertyCallback<CLabel>(CLabel *obj, const char *property)
{
	obj->needUpdate = true;
}

JS_PROPERTY(CLabel, UInt32, width)
JS_PROPERTY(CLabel, UInt32, height)
JS_PROPERTY(CLabel, String, text)
JS_PROPERTY(CLabel, String, textAlign)
JS_PROPERTY_BYFUNC(CLabel, String, fillStyle)
JS_PROPERTY_BYFUNC(CLabel, String, strokeStyle)
JS_PROPERTY_BYFUNC(CLabel, String, font)

static JSStaticValue jsStaticValues[] = {
	JS_PROPERTY_DEF(width),
	JS_PROPERTY_DEF(height),
	JS_PROPERTY_DEF(text),
	JS_PROPERTY_DEF(textAlign),
	JS_PROPERTY_DEF(fillStyle),
	JS_PROPERTY_DEF(strokeStyle),
	JS_PROPERTY_DEF(font),
	{0, 0, 0}
};

static JSStaticFunction jsStaticFunctions[] = {
	{0, 0}
};

JS_CLASS_EXPORT(CLabel, Label)

CLabel::CLabel()
{
	needUpdate = true;
	m_texture = 0;
	m_fontBold = false;
	m_fontItalic = false;

	m_POTWidth = m_POTHeight = width = height = 32;
	set_font("10px sans-serif");
	set_fillStyle("black");
	set_strokeStyle("black");
}

CLabel::~CLabel()
{
	if( m_texture )
	{
		glDeleteTextures(1, &m_texture);
	}
}

void CLabel::set_font(string font)
{
	//	[font style] [font weight] [font size] [font name]

	vector<string> segs;
	StringUtil::split(StringUtil::lower( StringUtil::trim(font) ), " \t", segs);
	if( segs.size() <= 0 )
	{
		return;
	}

	if( segs.size() >= 2 )
	{
		string fontSize = segs[segs.size() - 2];
		if( fontSize.size() < 3 )
		{
			return;
		}
		
		string fontSizeUnit = fontSize.substr(fontSize.size()-2, 2);
		if( fontSizeUnit != "px" && fontSizeUnit != "pt" && fontSizeUnit != "mm" && fontSizeUnit != "cm" && fontSizeUnit != "in" )
		{
			return;
		}

		if( atof(fontSize.c_str()) <= 0 )
		{
			return;
		}
	}
	
	m_font = StringUtil::lower( StringUtil::trim(font) );
	m_fontName = segs[segs.size() - 1];

	string fontSize = segs[segs.size() - 2];
	m_fontSizeUnit = fontSize.substr(fontSize.size()-2, 2);
	m_fontSize = atof(fontSize.c_str());
	
	if( segs.size() >= 3 )
	{
		string fontWeight = segs[segs.size() - 3];
		if( fontWeight == "bold" || fontWeight == "bolder" || atoi(fontWeight.c_str()) >= 600 )
		{
			m_fontBold = true;
		}
	}

	if( segs.size() >= 4 )
	{
		string fontStyle = segs[segs.size() - 4];
		if( fontStyle == "italic" )
		{
			m_fontItalic = true;
		}
	}

	needUpdate = true;
}

void CLabel::set_fillStyle(string fillStyle)
{
	unsigned int color = 0;
	if( StringUtil::convertHTMLColor(fillStyle, color) )
	{	
		m_fillStyle = StringUtil::lower( StringUtil::trim( fillStyle ) );
		m_fillColor = color;
	}

	needUpdate = true;
}

void CLabel::set_strokeStyle(string strokeStyle)
{
	unsigned int color = 0;
	if( StringUtil::convertHTMLColor(strokeStyle, color) )
	{	
		m_strokeStyle = StringUtil::lower( StringUtil::trim( strokeStyle ) );
		m_strokeColor = color;
	}

	needUpdate = true;
}

GLuint CLabel::getTexture()
{
	if( m_texture && !needUpdate )
	{
		return m_texture;
	}

	m_POTWidth = computePOT(width);
	m_POTHeight = computePOT(height);

	JniHelper jniHelper;

	JNIEnv *env = jniHelper.getEnv();

	// String text, int w, int h, int pw, int ph, String textAlign, String fontName, 
	//		float fontSize, String fontSizeUnit, int textColor, boolean bold, boolean italic
	jmethodID method = jniHelper.getMethod(g_jgl, "createTextBitmap", "(Ljava/lang/String;IIIILjava/lang/String;Ljava/lang/String;FLjava/lang/String;IZZ[B)V");
	if( env != NULL && method != NULL )
	{
		jstring jText = env->NewStringUTF( text.c_str() );
		jstring jTextAlign = env->NewStringUTF( textAlign.c_str() );
		jstring jFontName = env->NewStringUTF( m_fontName.c_str() );
		jstring jFontSizeUnit = env->NewStringUTF( m_fontSizeUnit.c_str() );

		unsigned int size = m_POTWidth * m_POTHeight * 4;
		unsigned char *data = new unsigned char[size];
		jbyteArray bytes = env->NewByteArray(size);
		
		env->CallVoidMethod(g_jgl, method, jText, width, height, m_POTWidth, m_POTHeight, jTextAlign, 
											jFontName, m_fontSize, jFontSizeUnit, m_fillColor, m_fontBold, m_fontItalic, bytes);
		env->GetByteArrayRegion(bytes, 0, size, (jbyte*)data);
		env->DeleteLocalRef(bytes);

		// 开始预乘
		for( int i=0; i<height; i++ )
		{
			for( int j=0; j<width; j++ )
			{
				unsigned char *pixel = (unsigned char *)(data + (i * m_POTWidth + j) * 4);
				*((unsigned int *)pixel) = CC_RGB_PREMULTIPLY_APLHA( pixel[0], pixel[1], pixel[2], pixel[3] );
			}
		}

		// 创建/更新Opengl ES Texture
		if( m_texture )
		{
			glDeleteTextures(1, &m_texture);
			m_texture = 0;
		}
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_POTWidth, m_POTHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		delete[] data;

		needUpdate = false;
	}

	return m_texture;
}
