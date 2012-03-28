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

	CCanvasContext *context = jsGetCObject<CCanvasContext>(args.Holder());

	if( !context || argc < 1 )
	{
		return v8::Undefined();
	}

	CImage *image = jsGetCObject<CImage>(args[0]->ToObject());
	if( !image )
	{
		return v8::Undefined();
	}

	float dx = 0;
	float dy = 0;
	float dw = image->get_width();
	float dh = image->get_height();
	float sx = 0;
	float sy = 0;
	float sw = image->get_width();
	float sh = image->get_height();

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

static v8::Handle<v8::Value> JS_drawLabel( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	CCanvasContext *context = jsGetCObject<CCanvasContext>(args.Holder());

	if( !context || argc < 1 )
	{
		return v8::Undefined();
	}

	CLabel *label = jsGetCObject<CLabel>(args[0]->ToObject());
	if( !label )
	{
		return v8::Undefined();
	}

	float dx = 0;
	float dy = 0;
	float dw = label->width;
	float dh = label->height;
	float sx = 0;
	float sy = 0;
	float sw = label->width;
	float sh = label->height;

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

	context->drawLabel(label, sx, sy, sw, sh, dx, dy, dw, dh);


	return v8::Undefined();
}

static v8::Handle<v8::Value> JS_fillRect( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	CCanvasContext *context = jsGetCObject<CCanvasContext>(args.Holder());

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

	CCanvasContext *context = jsGetCObject<CCanvasContext>(args.Holder());

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

	CCanvasContext *context = jsGetCObject<CCanvasContext>(args.Holder());

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

	CCanvasContext *context = jsGetCObject<CCanvasContext>(args.Holder());

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

	CCanvasContext *context = jsGetCObject<CCanvasContext>(args.Holder());

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
	JS_FUNCTION_DEF(drawLabel),
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
CCanvasContext::CCanvasContext()
{ 
	globalAlpha = 1.0; 
	m_image = NULL;
	
	set_fillStyle("black");
	set_strokeStyle("black");
}

void CCanvasContext::set_fillStyle(string fillStyle)
{
	unsigned int color = 0;
	if( StringUtil::convertHTMLColor(fillStyle, color) )
	{	
		m_fillStyle = StringUtil::lower( StringUtil::trim( fillStyle ) );
		m_fillColor.r = ((color & 0xff0000) >> 16) / 255.0;
		m_fillColor.g = ((color & 0xff00) >> 8) / 255.0;
		m_fillColor.b = (color & 0xff) / 255.0;
		m_fillColor.a = 1.0;
	}
}

void CCanvasContext::set_strokeStyle(string strokeStyle)
{
	unsigned int color = 0;
	if( StringUtil::convertHTMLColor(strokeStyle, color) )
	{	
		m_strokeStyle = StringUtil::lower( StringUtil::trim( strokeStyle ) );
		m_strokeColor.r = ((color & 0xff0000) >> 16) / 255.0;
		m_strokeColor.g = ((color & 0xff00) >> 8) / 255.0;
		m_strokeColor.b = (color & 0xff) / 255.0;
		m_strokeColor.a = 1.0;
	}
}

void CCanvasContext::drawImage(CImage *image, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh)
{
	if( !image || !image->getTexture() )
	{
		return;
	}

	if( m_image && m_coords.size() > 0 && (image->getTexture() != m_image->getTexture()) )
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
}

void CCanvasContext::drawImageBatch(CImage *image, const vector<float> &coords)
{
	//LOG("drawImageBatch: %d", coords.size());
	int count = (int)(coords.size() / 8);
	if( !image || count <= 0 )
		return;

	//glPushMatrix();
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, image->getTexture());
	
	if( image->hasAlpha() )
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
		unsigned long POTWidth = image->POTWidth();
		unsigned long POTHeight = image->POTHeight();

		textureCoords[i*12] = sx / POTWidth;
		textureCoords[i*12+1] = sy / POTHeight;
		
		textureCoords[i*12+2] = (sx + sw) / POTWidth;
		textureCoords[i*12+3] = sy / POTHeight;
		
		textureCoords[i*12+4] = sx / POTWidth;
		textureCoords[i*12+5] = (sy + sh) / POTHeight;
		
		textureCoords[i*12+6] = (sx + sw) / POTWidth;
		textureCoords[i*12+7] = sy / POTHeight;
		
		textureCoords[i*12+8] = sx / POTWidth;
		textureCoords[i*12+9] = (sy + sh) / POTHeight;
		
		textureCoords[i*12+10] = (sx + sw) / POTWidth;
		textureCoords[i*12+11] = (sy + sh) / POTHeight;
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

void CCanvasContext::drawLabel(CLabel *label, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh)
{
	if( !label || !label->getTexture() )
	{
		return;
	}

	cleanDrawImage();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, label->getTexture());

	// 已经做了预乘
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	GLfloat vertices[] = {
		dx,		dy,
		dx+dw,	dy,
		dx,		dy+dh,
		dx+dw,	dy+dh,
	};
	
	unsigned long POTWidth = label->POTWidth();
	unsigned long POTHeight = label->POTHeight();

	GLfloat textureCoords[] = {
		sx/POTWidth,		sy/POTHeight,
		(sx+sw)/POTWidth,	sy/POTHeight,
		sx/POTWidth,		(sy+sh)/POTHeight,
		(sx+sw)/POTWidth,	(sy+sh)/POTHeight,
	};

	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glTexCoordPointer(2, GL_FLOAT, 0, textureCoords);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisable(GL_VERTEX_ARRAY);
	glDisable(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
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
	cleanDrawImage();

	glColor4f(m_fillColor.r, m_fillColor.g, m_fillColor.b, globalAlpha);
	
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
	cleanDrawImage();

	glTranslatef(x, y, 0);
}

void CCanvasContext::scale(float scaleX, float scaleY)
{
	cleanDrawImage();

	glScalef(scaleX, scaleY, 0);
}

void CCanvasContext::rotate(float angle)
{
	cleanDrawImage();

	glRotatef(angle, 0, 0, 1.0);
}

void CCanvasContext::setTransform(float m11, float m12, float m21, float m22, float dx, float dy)
{
	cleanDrawImage();

	const GLfloat matrix[] = {
		m11,	m12,	0,	dx,
		m21,	m22,	0,	dy,
		0,		0,		0,	0,
		0,		0,		0,	0
	};
	
	glMultMatrixf(matrix);
}
