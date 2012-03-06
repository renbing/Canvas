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

#include <jni.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <string.h>
#include <pthread.h>

#include <png.h>
#include <zip.h>
#include <curl/curl.h>

#include "global.h"

#include "test.h"
#include "js.h"
#include "canvas.h"
#include "network.h"
#include "timer.h"

// 测试变量
extern void test();
extern const char *site;

// Android资源文件zip读取全局变量
zip *apkArchive;

// OpenGL ES全局变量
pthread_mutex_t gl_mutex;

// V8 全局变量
Persistent<Function> jsMainLoop;

//  暴露给Java的函数接口
extern "C" {

/* Canvas 类JNI函数
*	stringFromJNI(void) 		测试JNI调用
*	nativeInit(String apkPath)	主程序初始化工作
*	nativeDone(void)			主程序结束清理工作
*/
jstring Java_com_woyouquan_Canvas_stringFromJNI( JNIEnv *env, jobject obj )
{
	return env->NewStringUTF("Hello from NDK !");
}

void Java_com_woyouquan_Canvas_nativeInit( JNIEnv *env, jclass cls, jstring apkPath )
{
	curl_global_init(CURL_GLOBAL_NOTHING);

	const char *strApkPath = env->GetStringUTFChars(apkPath, 0);
	LOG("apk path: %s", strApkPath);

	apkArchive = zip_open(strApkPath, 0, NULL);
	env->ReleaseStringUTFChars(apkPath, strApkPath);

	if( apkArchive == NULL )
	{
		LOG("Loading APK failed");
	}

	test();

	LOG("canvas init");
}

void Java_com_woyouquan_Canvas_nativeDone( JNIEnv *env )
{
	zip_close(apkArchive);

	if( !jsMainLoop.IsEmpty() )
	{
		jsMainLoop.Dispose();
	}

	CTimer::getInstance()->clean();
	CV8Context::getInstance()->clean();

	curl_global_cleanup();
	
	LOG("canvas done");
}


/* EAGLRenderer 类JNI函数
*	nativeInit(void)						OpenGL ES初始化
*	nativeResize(int width, int height)		OpenGL ES窗口大小改变
*	nativeRenderer(void)					OpenGL ES逐帧绘图
*/
void Java_com_woyouquan_EAGLRenderer_nativeInit( JNIEnv *env )
{
	LOG("opengl init");
}

void Java_com_woyouquan_EAGLRenderer_nativeResize( JNIEnv *env, jobject obj, jint width, jint height )
{
	LOG("opengl resize: width=%d height=%d", width, height);
	CCanvas::getInstance()->width = width;
	CCanvas::getInstance()->height = height;

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0, width, -height, 0, -0.01, 100);
	glMatrixMode(GL_MODELVIEW);

	glClearColor(1,1,1,1.0);
	glColor4f(1,1,1,1);

	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	Locker locker;
	CV8Context::getInstance()->run(site);
}

void Java_com_woyouquan_EAGLRenderer_nativeRender( JNIEnv *env )
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	Locker locker;

	AsyncDownloadQueue::getInstance()->checkComplete();
	if( !jsMainLoop.IsEmpty() )
	{
		CV8Context::getInstance()->callJSFunction(jsMainLoop, 0, 0);
	}

	//drawTest();
}

void Java_com_woyouquan_TouchListener_nativeDown( JNIEnv *env, jobject obj, jfloat x, jfloat y )
{
	CCanvas::getInstance()->onTouch(TOUCH_DOWN, x, y);
}

void Java_com_woyouquan_TouchListener_nativeMove( JNIEnv *env, jobject obj, jfloat x, jfloat y )
{
	CCanvas::getInstance()->onTouch(TOUCH_MOVE, x, y);
}

void Java_com_woyouquan_TouchListener_nativeUp( JNIEnv *env, jobject obj, jfloat x, jfloat y )
{
	CCanvas::getInstance()->onTouch(TOUCH_UP, x, y);
}

};
