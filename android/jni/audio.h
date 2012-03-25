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

class CAudio
{
	private:
		bool m_loop;
		bool m_autoplay;
		string m_src;
		jobject m_player;
	
	public:
		JS_CLASS_EXPORT_DEF(CAudio)
		
		CAudio();
		CAudio(const string &src);
		~CAudio();

		void set_src(string src);
		string & get_src() { return m_src; }

		void set_loop(bool loop);
		bool & get_loop() { return m_loop; }

		void set_autoplay(bool autoplay);
		bool & get_autoplay() { return m_autoplay; }
		
		void play();
		void pause();
};
