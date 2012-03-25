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

#include <pthread.h>
#include <GLES/gl.h>
#include <GLES/glext.h>

#include "context.h"
#include "stringutil.h"

static v8::Handle<v8::Value> JS_drawImage( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	//v8::Local<v8::Object> self = args.Holder();
	//v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
	//CCanvasContext *context = static_cast<CCanvasContext *>(wrap->Value());

	CCanvasContext *context = getCObjct<CCanvasContext>(args.Holder());

	if( !context || argc < 1 )
	{
		return v8::Undefined();
	}

	CImage *image = getCObjct<CImage>(args[0]->ToObject());
	if( !image )
	{
		return v8::Undefined();
	}

	float dx = 0;
	float dy = 0;
	float dw = image->width;
	float dh = image->height;
	float sx = 0;
	float sy = 0;
	float sw = image->width;
	float sh = image->height;

	if( argc >= 3 )
	{
		dx = args[1]->NumberValue();
		dy = args[2]->NumberValue();
	}

	if( argc >= 5 )
	{
		dw = args[3]->NumberValue();
		dh = args[4]->NumberValue();
	}

	if( argc >= 9 )
	{
		sx = dx;
		sy = dy;
		sw = dw;
		sh = dh;

		dx = args[5]->NumberValue();
		dy = args[6]->NumberValue();
		dw = args[7]->NumberValue();
		dh = args[8]->NumberValue();
	}

	context->drawImage(image, sx, sy, sw, sh, dx, dy, dw, dh);


	return v8::Undefined();
}

static v8::Handle<v8::Value> JS_fillRect( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	v8::Local<v8::Object> self = args.Holder();
	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
	CCanvasContext *context = static_cast<CCanvasContext *>(wrap->Value());

	if( !context || argc < 4 )
	{
		return v8::Undefined();
	}

	float x = args[0]->NumberValue();
	float y = args[1]->NumberValue();
	float width = args[2]->NumberValue();
	float height = args[3]->NumberValue();

	context->fillRect(x, y, width, height);

	return v8::Undefined();
}

static v8::Handle<v8::Value> JS_translate( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	v8::Local<v8::Object> self = args.Holder();
	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
	CCanvasContext *context = static_cast<CCanvasContext *>(wrap->Value());

	if( !context || argc < 2 )
	{
		return v8::Undefined();
	}

	float x = args[0]->NumberValue();
	float y = args[1]->NumberValue();

	context->translate(x, y);

	return v8::Undefined();
}

static v8::Handle<v8::Value> JS_scale( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	v8::Local<v8::Object> self = args.Holder();
	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
	CCanvasContext *context = static_cast<CCanvasContext *>(wrap->Value());

	if( !context || argc < 2 )
	{
		return v8::Undefined();
	}

	float scaleX = args[0]->NumberValue();
	float scaleY = args[1]->NumberValue();

	context->scale(scaleX, scaleY);

	return v8::Undefined();
}

static v8::Handle<v8::Value> JS_rotate( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	v8::Local<v8::Object> self = args.Holder();
	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
	CCanvasContext *context = static_cast<CCanvasContext *>(wrap->Value());

	if( !context || argc < 1 )
	{
		return v8::Undefined();
	}

	float angle = args[0]->NumberValue();

	context->rotate(angle);

	return v8::Undefined();
}

static v8::Handle<v8::Value> JS_setTransform( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	v8::Local<v8::Object> self = args.Holder();
	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
	CCanvasContext *context = static_cast<CCanvasContext *>(wrap->Value());

	if( !context || argc < 6 )
	{
		return v8::Undefined();
	}

	float m11 = args[0]->NumberValue();
	float m12 = args[1]->NumberValue();
	float m21 = args[2]->NumberValue();
	float m22 = args[3]->NumberValue();
	float dx = args[4]->NumberValue();
	float dy = args[5]->NumberValue();

	context->setTransform(m11, m12, m21, m22, dx, dy);

	return v8::Undefined();
}

JS_SIMPLE_FUNCTION(CCanvasContext, save)
JS_SIMPLE_FUNCTION(CCanvasContext, restore)

JS_PROPERTY(CCanvasContext, Number, globalAlpha)
JS_PROPERTY_BYFUNC(CCanvasContext, string, fillStyle)

static JSStaticValue jsStaticValues[] = {
	JS_PROPERTY_DEF(fillStyle),
	JS_PROPERTY_DEF(globalAlpha),
	{0, 0, 0}
};

static JSStaticFunction jsStaticFunctions[] = {
	JS_FUNCTION_DEF(drawImage),
//	JS_FUNCTION_DEF(drawImageBatch),
	JS_FUNCTION_DEF(fillRect),
	JS_FUNCTION_DEF(save),
	JS_FUNCTION_DEF(restore),
	JS_FUNCTION_DEF(translate),
	JS_FUNCTION_DEF(scale),
	JS_FUNCTION_DEF(rotate),
	JS_FUNCTION_DEF(setTransform),
	{"transform", JS_setTransform}, 
	{0, 0}
};

JS_CLASS_EXPORT(CCanvasContext, Canvas)

void CCanvasContext::set_fillStyle(string fillStyle)
{
	m_fillStyle = StringUtil::lower( StringUtil::trim( fillStyle ) );

	static const char *normalColors[] = {
		"#000000", "black",	"#c0c0c0", "silver",
		"#ff0000", "red",	"#0000ff", "blue",
		"#ffffff", "white",	"#ffff00", "yellow",
		"#00ff00", "green",	"#00ffff", "aqua",
	};

	int normalCorlorCount = sizeof(normalColors)/sizeof(char *);

	if( m_fillStyle.empty() )
	{
		return;
	}
	
	if( !StringUtil::startWith(m_fillStyle, "#") || !StringUtil::startWith(m_fillStyle, "rgb") )
	{
		// 非#或者rgb开头
		
		for( int i=0; i<normalCorlorCount/2; i++ )
		{
			if( m_fillStyle == normalColors[i*2+1] )
			{
				m_fillStyle = normalColors[i];
				break;
			}
		}
	}
	
	int r,g,b;

	if( StringUtil::startWith(m_fillStyle, "#") )
	{
		if( m_fillStyle.size() > 4 )
		{
			// #ffff00
			r = StringUtil::getHexInt(m_fillStyle, 1, 2);
			g = StringUtil::getHexInt(m_fillStyle, 3, 2);
			b = StringUtil::getHexInt(m_fillStyle, 5, 2);
		}
		else
		{
			// #ff0
			r = StringUtil::getHexInt(m_fillStyle, 1, 1);
			r = r*16+r;
			g = StringUtil::getHexInt(m_fillStyle, 2, 1);
			g = g*16 + g;
			b = StringUtil::getHexInt(m_fillStyle, 3, 1);
			b = b*16 + b;
		}
	}
	else if( StringUtil::startWith(m_fillStyle, "rgb") )
	{
		sscanf(m_fillStyle.c_str(), "rgb(%d,%d,%d)", &r, &g, &b);
	}

	m_r = r/255.0; m_g = g/255.0; m_b = b/255.0;
}

string & CCanvasContext::get_fillStyle()
{
	return m_fillStyle;
}

void CCanvasContext::drawImage(CImage *image, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh)
{
	if( !image || !image->texture )
	{
		return;
	}

	if( m_image && m_coords.size() > 0 && (image->texture != m_image->texture) )
	{
		drawImageBatch(m_image, m_coords);
		m_coords.clear();
	}

	m_coords.push_back(sx);
	m_coords.push_back(sy);
	m_coords.push_back(sw);
	m_coords.push_back(sh);
	m_coords.push_back(dx);
	m_coords.push_back(dy);
	m_coords.push_back(dw);
	m_coords.push_back(dh);

	m_image = image;
	return;

	//LOG("drawImage: %s %d %d %lu %f %f %f %f %f %f %f %f", image->get_src().c_str(), image->hasAlpha, image->texture, pthread_self(), sx, sy, sw, sh, dx, dy, dw, dh);

	//glLoadIdentity();
	//glPushMatrix();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, image->texture);

	if( image->hasAlpha )
	{
		// 有alpha的PNG已经做了预乘
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
	}


	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	GLfloat vertices[] = {
		dx,		dy,
		dx+dw,	dy,
		dx,		dy+dh,
		dx+dw,	dy+dh,
	};

	GLfloat textureCoords[] = {
		sx/image->POTWidth,		sy/image->POTHeight,
		(sx+sw)/image->POTWidth,	sy/image->POTHeight,
		sx/image->POTWidth,		(sy+sh)/image->POTHeight,
		(sx+sw)/image->POTWidth,	(sy+sh)/image->POTHeight,
	};

	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glTexCoordPointer(2, GL_FLOAT, 0, textureCoords);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisable(GL_VERTEX_ARRAY);
	glDisable(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	//glPopMatrix();
}

void CCanvasContext::drawImageBatch(CImage *image, const vector<float> &coords)
{
	//LOG("drawImageBatch: %d", coords.size());
	int count = (int)(coords.size() / 8);
	if( !image || count <= 0 )
		return;

	//glPushMatrix();
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, image->texture);
	
	if( image->hasAlpha )
	{
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
	}
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	int vertexCount = count * 6;
	
	static int maxVertex = 100;
	static GLfloat *vertices = NULL;
	static GLfloat *textureCoords = NULL;
	if( !vertices )
	{
		vertices = (GLfloat *) malloc(maxVertex * 2 * sizeof(GLfloat));
	}
	if( !textureCoords )
	{
		textureCoords = (GLfloat *) malloc(maxVertex * 2 * sizeof(GLfloat));
	}
	
	if( vertexCount > maxVertex )
	{
		int newMaxVertex = maxVertex * 2;
		if( vertexCount > newMaxVertex )
		{
			newMaxVertex = vertexCount;
		}
		GLfloat *newVertexBuf = (GLfloat *) malloc(newMaxVertex * 2 * sizeof(GLfloat));
		GLfloat *newTextureCoordBuf = (GLfloat *) malloc(newMaxVertex * 2 * sizeof(GLfloat));
		
		free(vertices);
		free(textureCoords);
		
		vertices = newVertexBuf;
		textureCoords = newTextureCoordBuf;
		maxVertex = newMaxVertex;
	}

	for( int i=0; i<count; i++ )
	{
		float sx = coords[i*8];
		float sy = coords[i*8+1];
		float sw = coords[i*8+2];
		float sh = coords[i*8+3];
		
		float dx = coords[i*8+4];
		float dy = coords[i*8+5];
		float dw = coords[i*8+6];
		float dh = coords[i*8+7];
		
		// 6个点的订单坐标，其中2,3点和4,5点相同
		
		vertices[i*12] = dx;
		vertices[i*12+1] = dy;
		
		vertices[i*12+2] = dx + dw;
		vertices[i*12+3] = dy;
		
		vertices[i*12+4] = dx;
		vertices[i*12+5] = dy + dh;
		
		vertices[i*12+6] = dx + dw;
		vertices[i*12+7] = dy;
		
		vertices[i*12+8] = dx;
		vertices[i*12+9] = dy + dh;
		
		vertices[i*12+10] = dx + dw;
		vertices[i*12+11] = dy + dh;
		
		// 6个点的纹理坐标，其中2,3点和4,5点相同
		textureCoords[i*12] = sx / image->POTWidth;
		textureCoords[i*12+1] = sy / image->POTHeight;
		
		textureCoords[i*12+2] = (sx + sw) / image->POTWidth;
		textureCoords[i*12+3] = sy / image->POTHeight;
		
		textureCoords[i*12+4] = sx / image->POTWidth;
		textureCoords[i*12+5] = (sy + sh) / image->POTHeight;
		
		textureCoords[i*12+6] = (sx + sw) / image->POTWidth;
		textureCoords[i*12+7] = sy / image->POTHeight;
		
		textureCoords[i*12+8] = sx / image->POTWidth;
		textureCoords[i*12+9] = (sy + sh) / image->POTHeight;
		
		textureCoords[i*12+10] = (sx + sw) / image->POTWidth;
		textureCoords[i*12+11] = (sy + sh) / image->POTHeight;
	}

	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glTexCoordPointer(2, GL_FLOAT, 0, textureCoords);
	
	glDrawArrays(GL_TRIANGLES, 0, vertexCount);
	
	//free(vertices);
	//free(textureCoords);
	
	glDisable(GL_VERTEX_ARRAY);
	glDisable(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	//glPopMatrix();
}

void CCanvasContext::cleanDrawImage()
{
	if( m_image && m_coords.size() > 0 )
	{
		drawImageBatch(m_image, m_coords);
		m_image = NULL;
		m_coords.clear();
	}
}

void CCanvasContext::clearRect(float x, float y, float width, float height)
{
}

void CCanvasContext::fillRect(float x, float y, float width, float height)
{
	glColor4f(m_r, m_g, m_b, globalAlpha);
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	
	const GLfloat vertices[] = {
		x,			y,
		x+width,	y,
		x,			y+height,
		x+width,	y+height,
	};
	
	glEnable(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glDisable(GL_BLEND);
}

void CCanvasContext::save()
{
	glPushMatrix();
}

void CCanvasContext::restore()
{
	glPopMatrix();
}

void CCanvasContext::translate(float x, float y)
{
	glTranslatef(x, y, 0);
}

void CCanvasContext::scale(float scaleX, float scaleY)
{
	glScalef(scaleX, scaleY, 0);
}

void CCanvasContext::rotate(float angle)
{
	glRotatef(angle, 0, 0, 1.0);
}

void CCanvasContext::setTransform(float m11, float m12, float m21, float m22, float dx, float dy)
{
	const GLfloat matrix[] = {
		m11,	m12,	0,	dx,
		m21,	m22,	0,	dy,
		0,		0,		0,	0,
		0,		0,		0,	0
	};
	
	glMultMatrixf(matrix);
}
