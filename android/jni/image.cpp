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
#include <zip.h>

#include "global.h"
#include "network.h"
#include "image.h"
#include "js.h"
#include "stringutil.h"
#include "urllib.h"

extern zip *apkArchive;

JS_PROPERTY_READONLY_BYFUNC(CImage, UInt32, width)
JS_PROPERTY_READONLY_BYFUNC(CImage, UInt32, height)
JS_PROPERTY_BYFUNC(CImage, String, src)
JS_PROPERTY(CImage, Function, onload)

static JSStaticValue jsStaticValues[] = {
	JS_PROPERTY_READONLY_DEF(width),
	JS_PROPERTY_READONLY_DEF(height),
	JS_PROPERTY_DEF(src),
	JS_PROPERTY_DEF(onload),
	{0, 0, 0}
};

static JSStaticFunction jsStaticFunctions[] = {
	{0, 0}
};

JS_CLASS_EXPORT(CImage, Image)

static void png_zip_read(png_structp png, png_bytep data, png_size_t size)
{
	zip_file **zfp = (zip_file **)png_get_io_ptr(png);
	zip_fread(*zfp, data, size);    

//	LOG("png zip read\n");
}

static void png_raw_read(png_structp png, png_bytep data, png_size_t size)
{
	unsigned char **raws = (unsigned char **)png_get_io_ptr(png);
	memcpy(data, *raws, size);
	*raws = *raws + size;

//	LOG("png raws read\n");
}

static void imageLoadedCallback(Downloader *downloader, void *arg)
{
	CImage *image = (CImage *)arg;
	if( downloader->status() == 200 )
	{
		image->createTextureWithPngData(RAW, downloader->receivedData(), png_raw_read);

		if( !image->onload.IsEmpty() )
		{
			CV8Context::getInstance()->callJSFunction(image->onload, 0, 0);
		}
	}

	delete downloader;
}

CImage::CImage()
{
	m_width = 0;
	m_height = 0;
	m_POTWidth = 0;
	m_POTHeight = 0;
	m_hasAlpha = true;

	m_texture = 0;
}

CImage::~CImage()
{
	if( m_texture )
	{
		LOG("delete texture");
		glDeleteTextures(1, &m_texture);
	}
}

void CImage::set_src(string src)
{
	m_src = src;

	string fullPath = URLUtil::url2Absolute( CV8Context::getInstance()->path() , m_src );
	//LOG("image src:%s", fullPath.c_str());
	
	if( StringUtil::startWith(fullPath, "http://") )
	{
		AsyncDownloadQueue::getInstance()->downloadURL(&fullPath, false, NULL, imageLoadedCallback, this);
	}
	else
	{
		zip_file *file = zip_fopen(apkArchive, fullPath.c_str(), 0);
		if( !file )
		{
			LOG("Error opening %s from APK", fullPath.c_str());
			return;
		}
		
		createTextureWithPngData(ZIP, file, png_zip_read);

		zip_fclose(file);
	}
}

void CImage::createTextureWithPngData(PNGSOURCE source, const void *input, png_rw_ptr read_fn)
{
	string imagePath = URLUtil::url2Absolute( CV8Context::getInstance()->path() , m_src );
	const char *filename = imagePath.c_str();

	do{
		zip_file *zfile = (zip_file *)input;
		const void *raws = input;
		
		png_byte header[8];

		if( source == ZIP )
		{
			zip_fread(zfile, header, 8);
		}
		else if( source == RAW )
		{
			memcpy(header, raws, 8);
		}

		// 读取图片头,判断是否PNG文件
		if( png_sig_cmp(header, 0, 8) )
		{
			LOG("Not a png file : %s", filename);
			break;
		}

		// 创建PNG读取结构
		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if( !png_ptr )
		{
			LOG("Unable to create png struct : %s", filename);
			break;
		}

		// 创建PNG信息结构
		png_infop info_ptr = png_create_info_struct(png_ptr);
		if( !info_ptr )
		{
			png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
			LOG("Unable to create png info : %s", filename);
			break;
		}

		if( setjmp(png_jmpbuf(png_ptr)) )
		{
			LOG("Error during setjmp : %s", filename);
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
			break;
		}
		
		// 设置数据读取函数
		void *inputCopy = (void *)input;
		png_set_read_fn(png_ptr, &inputCopy, read_fn);

		// 跳过头部8个字节,raw读取方法不需要跳过
		if( source == ZIP )
		{
			png_set_sig_bytes(png_ptr, 8);
		}

		// 设置PNG数据宽度处理 RGB/RGBA 24/32bit
		//png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_PACKING
		//								| PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_GRAY_TO_RGB, 0);
		png_read_info(png_ptr, info_ptr);

		// 读取图片信息
		int bit_depth, color_type;
		png_uint_32 twidth, theight;

		png_get_IHDR(png_ptr, info_ptr, &twidth, &theight, &bit_depth, &color_type, NULL, NULL, NULL);
		m_hasAlpha = ( color_type & PNG_COLOR_MASK_ALPHA ) ? true:false;

		LOG("PNG width=%u height=%u bit_depth=%d alpha=%d", twidth, theight, bit_depth, m_hasAlpha);
		

		// Update the png info struct.
		//png_read_update_info(png_ptr, info_ptr);

		int rowbytes = png_get_rowbytes(png_ptr, info_ptr);
		LOG("PNG rowbytes:%d", rowbytes);
		
		int bytes_per_component = m_hasAlpha ? 4:3;

		m_width = twidth;
		m_height = theight;
		m_POTWidth = computePOT(m_width);
		m_POTHeight = computePOT(m_height);

		png_byte *image_data = new png_byte[m_POTWidth * m_POTHeight * bytes_per_component];
		if( !image_data )
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
			LOG("Unable to allocate image_data while loading %s ", filename);
			break;
		}

		// 创建并设置行指针
		png_bytep *row_pointers = new png_bytep[theight];
		if( !row_pointers )
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
			delete[] image_data;
			LOG("Unable to allocate row_pointer while loading %s ", filename);
			break;
		}

		for( int i = 0; i < theight; i++ )
		{
			row_pointers[i] = image_data + i * m_POTWidth * bytes_per_component;
		}

		png_read_image(png_ptr, row_pointers);

		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
		delete[] row_pointers;

		// 如果有alpha值,开始预乘
		if( m_hasAlpha )
		{
			for( int i=0; i<theight; i++ )
			{
				for( int j=0; j<twidth; j++ )
				{
					unsigned char *pixel = (unsigned char *)(image_data + (i * m_POTWidth + j) * bytes_per_component);
					*((unsigned int *)pixel) = CC_RGB_PREMULTIPLY_APLHA( pixel[0], pixel[1], pixel[2], pixel[3] );
				}
			}
		}

		// 创建Opengl ES Texture
		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
		if( m_hasAlpha )
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_POTWidth, m_POTHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_POTWidth, m_POTHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
		}

		LOG("Loading PNG: %s finish",filename);

		delete[] image_data;
	}while(false);
}
