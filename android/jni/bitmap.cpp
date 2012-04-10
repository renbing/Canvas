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

#include "bitmap.h"

extern JavaVM *g_jvm;
extern jobject g_jgl;

template<>
inline v8::Handle<v8::Value> jsConstructor<CBitmap>( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	if( args.IsConstructCall() )
	{
		if( argc < 2 )
		{
			return v8::Undefined();
		}

		CBitmap *obj = NULL;

		unsigned long w = args[0]->Uint32Value();
		unsigned long h = args[1]->Uint32Value();
		obj = new CBitmap(w, h);

		v8::Persistent<v8::Object> self = v8::Persistent<v8::Object>::New(args.Holder());
		self.MakeWeak(obj, jsDestructor<CBitmap>);
		self->SetInternalField(0, v8::External::New(obj));

		return handleScope.Close(self);
	}

	return v8::Undefined();
}

static v8::Handle<v8::Value> JS_moveTo( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	CBitmap *bitmap = jsGetCObject<CBitmap>(args.Holder());

	if( !bitmap || argc < 2 )
	{
		return v8::Undefined();
	}

	float x = args[0]->NumberValue();
	float y = args[1]->NumberValue();

	bitmap->moveTo(x, y);

	return v8::Undefined();
}

static v8::Handle<v8::Value> JS_lineTo( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	CBitmap *bitmap = jsGetCObject<CBitmap>(args.Holder());

	if( !bitmap || argc < 2 )
	{
		return v8::Undefined();
	}

	float x = args[0]->NumberValue();
	float y = args[1]->NumberValue();

	bitmap->lineTo(x, y);

	return v8::Undefined();
}

static v8::Handle<v8::Value> JS_clearRect( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	CBitmap *bitmap = jsGetCObject<CBitmap>(args.Holder());

	if( !bitmap || argc < 4 )
	{
		return v8::Undefined();
	}

	float x = args[0]->NumberValue();
	float y = args[1]->NumberValue();
	float w = args[2]->NumberValue();
	float h = args[3]->NumberValue();

	bitmap->clearRect(x, y, w, h);

	return v8::Undefined();
}

static v8::Handle<v8::Value> JS_fillText( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	CBitmap *bitmap = jsGetCObject<CBitmap>(args.Holder());

	if( !bitmap || argc < 3 )
	{
		return v8::Undefined();
	}
	
	string text;
	text = ValueCast::Cast(text, args[0]);

	float x = args[1]->NumberValue();
	float y = args[2]->NumberValue();

	float maxWidth = bitmap->get_width();
	if( argc >= 4 )
	{
		maxWidth = args[3]->NumberValue();
	}
	
	bitmap->fillText(text, x, y, maxWidth);

	return v8::Undefined();
}

JS_SIMPLE_FUNCTION(CBitmap, stroke);
JS_SIMPLE_FUNCTION(CBitmap, beginPath);
JS_SIMPLE_FUNCTION(CBitmap, closePath);

JS_PROPERTY_READONLY_BYFUNC(CBitmap, Uint32, width)
JS_PROPERTY_READONLY_BYFUNC(CBitmap, Uint32, height)
JS_PROPERTY(CBitmap, Number, lineWidth)
JS_PROPERTY(CBitmap, Number, miterLimit)
JS_PROPERTY(CBitmap, String, lineCap)
JS_PROPERTY(CBitmap, String, lineJoin)
JS_PROPERTY_BYFUNC(CBitmap, String, fillStyle)
JS_PROPERTY_BYFUNC(CBitmap, String, strokeStyle)
JS_PROPERTY_BYFUNC(CBitmap, String, font)
JS_PROPERTY(CBitmap, String, textAlign)

static JSStaticValue jsStaticValues[] = {
	JS_PROPERTY_READONLY_DEF(width),
	JS_PROPERTY_READONLY_DEF(height),
	JS_PROPERTY_DEF(lineWidth),
	JS_PROPERTY_DEF(miterLimit),
	JS_PROPERTY_DEF(lineCap),
	JS_PROPERTY_DEF(lineJoin),
	JS_PROPERTY_DEF(fillStyle),
	JS_PROPERTY_DEF(strokeStyle),
	JS_PROPERTY_DEF(font),
	JS_PROPERTY_DEF(textAlign),
	{0, 0, 0}
};

static JSStaticFunction jsStaticFunctions[] = {
	JS_FUNCTION_DEF(beginPath),
	JS_FUNCTION_DEF(closePath),
	JS_FUNCTION_DEF(moveTo),
	JS_FUNCTION_DEF(lineTo),
	JS_FUNCTION_DEF(stroke),
	JS_FUNCTION_DEF(clearRect),
	JS_FUNCTION_DEF(fillText),
	{0, 0}
};

JS_CLASS_EXPORT(CBitmap, Bitmap)

static CRect JavaRectToCRect(jobject jrect)
{
	JniHelper jniHelper;
	JNIEnv *env = jniHelper.getEnv();
	
	jfieldID fieldLeft = jniHelper.getField(jrect, "left", "I");
	jfieldID fieldRight = jniHelper.getField(jrect, "right", "I");
	jfieldID fieldTop = jniHelper.getField(jrect, "top", "I");
	jfieldID fieldBottom = jniHelper.getField(jrect, "bottom", "I");

	CRect rect;
	rect.x = env->GetIntField(jrect, fieldLeft);
	rect.y = env->GetIntField(jrect, fieldTop);
	rect.width = env->GetIntField(jrect, fieldRight) - rect.x;
	rect.height = env->GetIntField(jrect, fieldBottom) - rect.y;

	return rect;
}

CBitmap::CBitmap()
{
	m_POTWidth = m_POTHeight = m_width = m_height = 32;

	init();
}

CBitmap::CBitmap(unsigned long w, unsigned long h)
{
	m_width = w;
	m_height = h;
	m_POTWidth = computePOT(w);
	m_POTHeight = computePOT(h);

	init();
}

CBitmap::~CBitmap()
{
	if( m_texture )
	{
		glDeleteTextures(1, &m_texture);
	}

	JniHelper jniHelper;
	jniHelper.DeleteGlobalRef(m_jbitmap);
	
	if( m_buf )
	{
		delete[] m_buf;
	}
}

void CBitmap::init()
{
	m_buf = NULL;
	m_texture = 0;

	lineWidth = 1.0;
	miterLimit = 10.0;
	lineCap = "butt";
	lineJoin = "miter";
	textAlign = "left";

	set_font("14px sans");
	set_fillStyle("black");
	set_strokeStyle("black");
	
	// 创建bitmap对应的: opengl buf, java bitmap data, JavaBitmap object
	unsigned long size = m_POTWidth * m_POTHeight * 4;
	m_buf = new unsigned char[size];


	JniHelper jniHelper;
	JNIEnv *env = jniHelper.getEnv();

	jmethodID newJavaBitmap = jniHelper.getMethod(g_jgl, "newBitmap", "(II)Lcom/woyouquan/JavaBitmap;");
	if( env != NULL && newJavaBitmap != NULL )
	{
		jobject jbitmap = env->CallObjectMethod(g_jgl, newJavaBitmap, m_POTWidth, m_POTHeight);
		m_jbitmap = env->NewGlobalRef(jbitmap);
	}

	// 创建Opengl ES Texture
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &m_texture);

	updateTexture();
}

void CBitmap::set_font(string font)
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
	m_cfont.name = segs[segs.size() - 1];

	string fontSize = segs[segs.size() - 2];
	m_cfont.unit = fontSize.substr(fontSize.size()-2, 2);
	m_cfont.size = atof(fontSize.c_str());
	m_cfont.bold = m_cfont.italic = false;
	
	if( segs.size() >= 3 )
	{
		string fontWeight = segs[segs.size() - 3];
		if( fontWeight == "bold" || fontWeight == "bolder" || atoi(fontWeight.c_str()) >= 600 )
		{
			m_cfont.bold = true;
		}
	}

	if( segs.size() >= 4 )
	{
		string fontStyle = segs[segs.size() - 4];
		if( fontStyle == "italic" )
		{
			m_cfont.italic = true;
		}
	}
}

void CBitmap::set_fillStyle(string fillStyle)
{
	unsigned int color = 0;
	if( StringUtil::convertHTMLColor(fillStyle, color) )
	{	
		m_fillStyle = StringUtil::lower( StringUtil::trim( fillStyle ) );
		m_fillColor = color;
	}
}

void CBitmap::set_strokeStyle(string strokeStyle)
{
	unsigned int color = 0;
	if( StringUtil::convertHTMLColor(strokeStyle, color) )
	{	
		m_strokeStyle = StringUtil::lower( StringUtil::trim( strokeStyle ) );
		m_strokeColor = color;
	}
}

void CBitmap::beginPath()
{
	JniHelper jniHelper;
	JNIEnv *env = jniHelper.getEnv();

	jmethodID method = jniHelper.getMethod(m_jbitmap, "beginPath", "()V");
	if( env != NULL && method != NULL )
	{
		env->CallVoidMethod(m_jbitmap, method);
	}
}

void CBitmap::closePath()
{
	JniHelper jniHelper;
	JNIEnv *env = jniHelper.getEnv();

	jmethodID method = jniHelper.getMethod(m_jbitmap, "closePath", "()V");
	if( env != NULL && method != NULL )
	{
		env->CallVoidMethod(m_jbitmap, method);
	}
}

void CBitmap::moveTo(float x, float y)
{
	JniHelper jniHelper;
	JNIEnv *env = jniHelper.getEnv();

	jmethodID method = jniHelper.getMethod(m_jbitmap, "moveTo", "(FF)V");
	if( env != NULL && method != NULL )
	{
		env->CallVoidMethod(m_jbitmap, method, x, y);
	}
}

void CBitmap::lineTo(float x, float y)
{
	JniHelper jniHelper;
	JNIEnv *env = jniHelper.getEnv();

	jmethodID method = jniHelper.getMethod(m_jbitmap, "lineTo", "(FF)V");
	if( env != NULL && method != NULL )
	{
		env->CallVoidMethod(m_jbitmap, method, x, y);
	}
}

void CBitmap::stroke()
{
	JniHelper jniHelper;
	JNIEnv *env = jniHelper.getEnv();

	// String lineCap, String lineJoin, float lineWidth, float miterLimit, 
	// int fillColor, int strokeColor, boolean styleStroke
	jmethodID method = jniHelper.getMethod(m_jbitmap, "setContextState", "(Ljava/lang/String;Ljava/lang/String;FFIIZ)V");
	if( env != NULL && method != NULL )
	{
		jstring jLineCap = env->NewStringUTF( lineCap.c_str() );
		jstring jLineJoin = env->NewStringUTF( lineJoin.c_str() );
		jboolean isStroke = true;

		env->CallVoidMethod(m_jbitmap, method, jLineCap, jLineJoin, lineWidth, miterLimit, m_fillColor, m_strokeColor, isStroke); 
	}

	method = jniHelper.getMethod(m_jbitmap, "stroke", "()Landroid/graphics/Rect;");
	if( env != NULL && method != NULL )
	{
		jobject jrect = env->CallObjectMethod(m_jbitmap, method);
		updateTexture(jrect);
	}
}

void CBitmap::clearRect(float x, float y, float w, float h)
{
	int ix = roundf(x);
	int iy = roundf(y);
	int iw = roundf(w);
	int ih = roundf(h);

	JniHelper jniHelper;
	JNIEnv *env = jniHelper.getEnv();

	jmethodID method = jniHelper.getMethod(m_jbitmap, "clearRect", "(IIII)Landroid/graphics/Rect;");
	if( env != NULL && method != NULL )
	{
		jobject jrect = env->CallObjectMethod(m_jbitmap, method, ix, iy, iw, ih);
		updateTexture(jrect);
	}
}

void CBitmap::fillText(const string &text, float x, float y, float maxWidth)
{
	int ix = roundf(x);
	int iy = roundf(y);

	int iw = roundf(maxWidth);
	int ih = (m_height - iy);

	JniHelper jniHelper;
	JNIEnv *env = jniHelper.getEnv();

	// String text, int x, int y, int w, int h, String textAlign,String fontName,
	// float fontSize, String fontSizeUnit, int textColor, boolean bold, boolean italic
	jmethodID method = jniHelper.getMethod(m_jbitmap, "fillText", "(Ljava/lang/String;IIIILjava/lang/String;Ljava/lang/String;FLjava/lang/String;IZZ)Landroid/graphics/Rect;");
	if( env != NULL && method != NULL )
	{
		jstring jText = env->NewStringUTF( text.c_str() );
		jstring jTextAlign = env->NewStringUTF( textAlign.c_str() );
		jstring jFontName = env->NewStringUTF( m_cfont.name.c_str() );
		jstring jFontSizeUnit = env->NewStringUTF( m_cfont.unit.c_str() );

		jobject jrect = env->CallObjectMethod(m_jbitmap, method, jText, ix, iy, iw, ih, jTextAlign, jFontName, 
												m_cfont.size, jFontSizeUnit, m_fillColor, m_cfont.bold, m_cfont.italic);
		updateTexture(jrect);
	}
}

void CBitmap::updateTexture(jobject jrect)
{
	JniHelper jniHelper;
	JNIEnv *env = jniHelper.getEnv();
	
	CRect rect = {0, 0, m_POTWidth, m_POTHeight};
	if( jrect != NULL )
	{
		rect = JavaRectToCRect(jrect);
	}
	if( rect.x < 0 )
	{
		rect.x = 0;
	}
	if( rect.y < 0 )
	{
		rect.y = 0;
	}
	if( rect.x + rect.width > m_POTWidth )
	{
		rect.width = m_POTWidth - rect.x;
	}
	if( rect.y + rect.height > m_POTHeight )
	{
		rect.height = m_POTHeight - rect.y;
	}

	if( rect.x >= m_width || rect.y >= m_height || m_width <= 0 || m_height <= 0 )
	{
		return;
	}

	jmethodID method = jniHelper.getMethod(m_jbitmap, "getRectPixels", "(IIII)[B");
	if( method == NULL )
	{
		return;
	}
	jbyteArray bytes = (jbyteArray) env->CallObjectMethod(m_jbitmap, method, rect.x, rect.y, rect.width, rect.height);

	unsigned long size = rect.width * rect.height * 4;
	unsigned char *buf  = new unsigned char[size];

	env->GetByteArrayRegion(bytes, 0, size, (jbyte*)buf);
	
	// 开始预乘
	int width = rect.width;
	int height = rect.height;
	for( int i=0; i<height; i++ )
	{
		for( int j=0; j<width; j++ )
		{
			//ARGB
			unsigned char *pixel = (unsigned char *)(buf + (i * width + j) * 4);
			*((unsigned int *)pixel) = CC_RGB_PREMULTIPLY_APLHA( pixel[1], pixel[2], pixel[3], pixel[0] );
		}
	}

	// 更新Opengl ES Texture
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_texture);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		
	if( jrect == NULL )
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rect.width, rect.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	}
	else
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x, rect.y, rect.width, rect.height, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	}

	delete[] buf;
}
