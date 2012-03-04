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
#include <time.h>

#include <v8.h>
#include <vector>

using namespace v8;
using namespace std;


typedef struct
{
	timer_t tt;
	Persistent<Function> callback;
	bool loop;

} CTimerInfo;


class CTimer
{
	private:
		static CTimer *m_instance;

		vector<CTimerInfo *> m_timers;
	
	private:
		CTimer();
	
	public:
		~CTimer();
		static CTimer * getInstance();

		int createTimer(int msec, Persistent<Function> callback, bool loop);
		void clearInterval(int timerid);
		void update(int timerid);
		void clean();
};