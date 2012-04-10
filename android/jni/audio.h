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
 * @todo:
 * 		muted: true/false
 * 		volume: 0-1
 * 		ended: true/false
 * 		readyState: 1:HAVE_NOTHING 2:HAVE_METADATA 3.HAVE_CURRENT_DATA 4.HAVE_FUTURE_DATA 5.HAVE_ENOUGH_DATA
 *		attribute float currentTime
 *		setter raises (DOMException);
 *		readonly attribute float startTime;
 *		readonly attribute float duration;
 *		readonly attribute boolean paused;
 *		attribute float defaultPlaybackRate;
 *		attribute float playbackRate;
 *		readonly attribute TimeRanges played;
 *		readonly attribute TimeRanges seekable;
 *		readonly attribute unsigned short networkState: NETWORK_EMPTY = 0; NETWORK_IDLE = 1; NETWORK_LOADING = 2; NETWORK_NO_SOURCE = 3;
*/

#pragma once
#include <jni.h>

class CAudio
{
	private:
		bool m_loop;
		bool m_autoplay;
		bool m_muted;
		float m_volume;
		string m_src;
		jobject m_player;
	
	private:
		void init();
	
	public:
		JS_CLASS_EXPORT_DEF(CAudio)
		
		CAudio();
		CAudio(const string &src);
		~CAudio();

		void set_src(string src);
		const string & get_src() { return m_src; }

		void set_loop(bool loop);
		bool get_loop() { return m_loop; }

		void set_autoplay(bool autoplay);
		bool get_autoplay() { return m_autoplay; }

		void set_muted(bool muted);
		bool get_muted() { return m_muted; }

		void set_volume(float volume);
		float get_volume() { return m_volume; }
		
		void play();
		void pause();

};
