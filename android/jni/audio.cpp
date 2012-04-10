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
#include "urllib.h"
#include "js.h"
#include "jnihelper.h"

#include "audio.h"

extern JavaVM *g_jvm;
extern jobject g_jgl;

template<>
inline v8::Handle<v8::Value> jsConstructor<CAudio>( const v8::Arguments& args )
{
	v8::HandleScope handleScope;

	int argc = args.Length();

	if( args.IsConstructCall() )
	{
		CAudio *obj = NULL;

		if( argc <= 0 )
		{
			LOG("audio constructor");
			obj = new CAudio();
		}
		else
		{
			string src;
			src = ValueCast::Cast(src, args[0]);
			obj = new CAudio(src);
		}

		v8::Persistent<v8::Object> self = v8::Persistent<v8::Object>::New(args.Holder());
		self.MakeWeak(obj, jsDestructor<CAudio>);
		self->SetInternalField(0, v8::External::New(obj));

		return handleScope.Close(self);
	}

	return v8::Undefined();
}

JS_PROPERTY_BYFUNC(CAudio, Boolean, loop)
JS_PROPERTY_BYFUNC(CAudio, Boolean, autoplay)
JS_PROPERTY_BYFUNC(CAudio, String, src)
JS_PROPERTY_BYFUNC(CAudio, Boolean, muted)
JS_PROPERTY_BYFUNC(CAudio, Number, volume)

static JSStaticValue jsStaticValues[] = {
	JS_PROPERTY_DEF(loop),
	JS_PROPERTY_DEF(autoplay),
	JS_PROPERTY_DEF(src),
	JS_PROPERTY_DEF(muted),
	JS_PROPERTY_DEF(volume),
	{0, 0, 0}
};

JS_SIMPLE_FUNCTION(CAudio, play)
JS_SIMPLE_FUNCTION(CAudio, pause)

static JSStaticFunction jsStaticFunctions[] = {
	JS_FUNCTION_DEF(play),
	JS_FUNCTION_DEF(pause),
	{0, 0}
};

JS_CLASS_EXPORT(CAudio, Audio)

CAudio::CAudio()
{
	init();
}

void CAudio::init()
{
	m_autoplay = false;
	m_loop = false;
	m_player = NULL;
	m_muted = false;
	m_volume = 1.0;

	JniHelper jniHelper;
	JNIEnv *env = jniHelper.getEnv();
	jmethodID newAudioPlayer = jniHelper.getMethod(g_jgl, "newAudioPlayer", "()Lcom/woyouquan/JavaAudioPlayer;");
	if( env != NULL && newAudioPlayer != NULL )
	{
		jobject player = env->CallObjectMethod(g_jgl, newAudioPlayer);
		m_player = env->NewGlobalRef(player);
	}
}

CAudio::CAudio(const string &src)
{
	init();
	set_src(src);
}

CAudio::~CAudio()
{
	LOG("CAudio destructor: %s", m_src.c_str());
	if( m_player )
	{
		JniHelper jniHelper;

		JNIEnv *env = jniHelper.getEnv();
		jmethodID method = jniHelper.getMethod(m_player, "release", "()V");
		if( env != NULL && method != NULL )
		{
			LOG("CAudio release");
			env->CallVoidMethod(m_player, method);
		}
		jniHelper.DeleteGlobalRef(m_player);
	}
}

void CAudio::set_src(string src)
{
	m_src = src;

	if( !m_player )
	{
		return;
	}

	string fullPath = URLUtil::url2Absolute(CV8Context::getInstance()->path(), m_src);
	LOG("CAudio audio src:%s", fullPath.c_str());

	JniHelper jniHelper;

	JNIEnv *env = jniHelper.getEnv();
	jmethodID setSource = jniHelper.getMethod(m_player, "setSource", "(Ljava/lang/String;)V");
	if( env != NULL && setSource != NULL )
	{
		jstring path = env->NewStringUTF( fullPath.c_str() );
		env->CallVoidMethod(m_player, setSource, path);
	}
}

void CAudio::play()
{
	if( !m_player )
	{
		return;
	}

	JniHelper jniHelper;

	JNIEnv *env = jniHelper.getEnv();
	jmethodID method = jniHelper.getMethod(m_player, "play", "()V");
	if( env != NULL && method != NULL )
	{
		env->CallVoidMethod(m_player, method);
	}
}

void CAudio::pause()
{
	if( !m_player )
	{
		return;
	}

	JniHelper jniHelper;

	JNIEnv *env = jniHelper.getEnv();
	jmethodID method = jniHelper.getMethod(m_player, "pause", "()V");
	if( env != NULL && method != NULL )
	{
		env->CallVoidMethod(m_player, method);
	}
}

void CAudio::set_loop(bool loop)
{
	m_loop = loop;

	if( !m_player )
	{
		return;
	}

	JniHelper jniHelper;

	JNIEnv *env = jniHelper.getEnv();
	jmethodID method = jniHelper.getMethod(m_player, "setLoop", "(Z)V");
	if( env != NULL && method != NULL )
	{
		env->CallVoidMethod(m_player, method, loop);
	}
}

void CAudio::set_autoplay(bool autoplay)
{
	m_autoplay = autoplay;

	if( !m_player )
	{
		return;
	}

	JniHelper jniHelper;

	JNIEnv *env = jniHelper.getEnv();
	jmethodID method = jniHelper.getMethod(m_player, "setAutoplay", "(Z)V");
	if( env != NULL && method != NULL )
	{
		env->CallVoidMethod(m_player, method, autoplay);
	}
}

void CAudio::set_muted(bool muted)
{
	m_muted = muted;

	if( !m_player )
	{
		return;
	}

	float realVolume = m_muted ? 0.0 : m_volume;

	JniHelper jniHelper;

	JNIEnv *env = jniHelper.getEnv();
	jmethodID method = jniHelper.getMethod(m_player, "setVolume", "(F)V");
	if( env != NULL && method != NULL )
	{
		env->CallVoidMethod(m_player, method, realVolume);
	}
}


void CAudio::set_volume(float volume)
{
	m_volume = volume;

	if( !m_player )
	{
		return;
	}

	float realVolume = m_muted ? 0.0 : m_volume;

	JniHelper jniHelper;

	JNIEnv *env = jniHelper.getEnv();
	jmethodID method = jniHelper.getMethod(m_player, "setVolume", "(F)V");
	if( env != NULL && method != NULL )
	{
		env->CallVoidMethod(m_player, method, realVolume);
	}
}
