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
#include <jni.h>

extern JavaVM *g_jvm;

class JniHelper
{
	private:
		JNIEnv *m_env;
		bool m_needAttach;

	public:
		JniHelper()
		{
			m_needAttach = false;
			if( g_jvm->GetEnv((void **)&m_env, JNI_VERSION_1_4) == JNI_EDETACHED )
			{
				m_needAttach = true;	
			}

			if( m_needAttach && (g_jvm->AttachCurrentThread(&m_env, NULL) != JNI_OK) )
			{
				LOG("JniHelper AttachCurrentThread fail");
			}

			if( m_env == NULL )
			{
				LOG("JniHelper get JniEnv fail");
			}
		}

		~JniHelper()
		{
			if( m_needAttach )
			{
				g_jvm->DetachCurrentThread();
			}
			
		}

		jclass getClass(jobject obj)
		{
			if( m_env == NULL )
			{
				return NULL;
			}

			jclass cls = m_env->GetObjectClass(obj);
			if( cls == NULL )
			{
				LOG("Get Java class fail");
				return NULL;
			}

			return cls;
		}

		jmethodID getMethod(jobject obj, const char *methodName, const char *params)
		{
			if( m_env == NULL )
			{
				return NULL;
			}

			jclass cls = m_env->GetObjectClass(obj);
			if( cls == NULL )
			{
				LOG("Get Java class fail");
				return NULL;
			}

			jmethodID method = m_env->GetMethodID(cls, methodName, params);
			if( method == NULL )
			{
				LOG("Get Java method fail");
				return NULL;
			}

			return method;
		}

		jfieldID getField(jobject obj, const char *fieldName, const char *params)
		{
			if( m_env == NULL )
			{
				return NULL;
			}

			jclass cls = m_env->GetObjectClass(obj);
			if( cls == NULL )
			{
				LOG("Get Java class fail");
				return NULL;
			}

			jfieldID field = m_env->GetFieldID(cls, fieldName, params);
			if( field == NULL )
			{
				LOG("Get Java field fail");
				return NULL;
			}

			return field;
		}

		JNIEnv * getEnv() { return m_env; }

		void DeleteGlobalRef(jobject obj)
		{
			if( m_env == NULL || obj == NULL )
			{
				return;
			}

			m_env->DeleteGlobalRef(obj);
		}
};
