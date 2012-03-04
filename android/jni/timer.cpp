/***
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

#include <time.h>
#include <signal.h>
#include <pthread.h>

#include "global.h"
#include "js.h"
#include "timer.h"

pthread_mutex_t timer_mutex;

static void timerHandler(sigval_t v)
{
	int timerid = v.sival_int;

	//LOG("update timer %d", timerid);

	CTimer::getInstance()->update(timerid);
}

CTimer * CTimer::m_instance = NULL;

CTimer::CTimer()
{
}

CTimer::~CTimer()
{
	for( int i=0; i<m_timers.size(); i++ )
	{
		CTimerInfo *info = m_timers[i];

		timer_delete(info->tt);
		(info->callback).Dispose();

		delete info;
	}
}

CTimer * CTimer::getInstance()
{
	if( m_instance == NULL )
	{
		// 需要加锁在这个位置
		if( m_instance == NULL )
		{
			m_instance = new CTimer();
		}
	}

	return m_instance;
}

int CTimer::createTimer(int msec, Persistent<Function> callback, bool loop)
{
	
	CTimerInfo *info = new CTimerInfo;
	info->callback = callback;
	info->loop = loop;
	
	pthread_mutex_lock( &timer_mutex );

	m_timers.push_back(info);
	int timerid = m_timers.size() - 1;

	pthread_mutex_unlock( &timer_mutex );

	struct sigevent se; 
	struct itimerspec ts, ots; 

	memset(&se, 0, sizeof(se)); 

	se.sigev_notify = SIGEV_THREAD;
	se.sigev_notify_function = timerHandler;
	se.sigev_value.sival_int = timerid; 

	if( timer_create(CLOCK_REALTIME, &se, &(info->tt)) < 0 ) 
	{ 
		LOG("create timer fail");
		return -1;
	}

	ts.it_value.tv_sec   =   msec / 1000; 
	ts.it_value.tv_nsec   =   (long)(msec % 1000) * (1000000L)  ; 
	
	if( loop )
	{
		ts.it_interval.tv_sec   =   ts.it_value.tv_sec;
		ts.it_interval.tv_nsec   =   ts.it_value.tv_nsec;
	}
	else
	{
		ts.it_interval.tv_sec   =   0;
		ts.it_interval.tv_nsec   =   0;
	}

	if( timer_settime(info->tt, TIMER_ABSTIME, &ts, &ots) < 0 )
	{
		LOG("start timer fail");
		return -1;
	}
	
	return timerid;
}

void CTimer::clearInterval(int timerid)
{
	if( timerid < m_timers.size() )
	{
		CTimerInfo *info = m_timers[timerid];
		timer_delete(info->tt);
		(info->callback).Dispose();
	}
}

void CTimer::update(int timerid)
{
	if( timerid >= m_timers.size() )
	{
		return;
	}

	CTimerInfo *info = m_timers[timerid];
	
	Locker locker;

	Persistent<Function> callback = info->callback;
	if( callback.IsEmpty() )
	{
		return;
	}

	CV8Context::getInstance()->callJSFunction(callback, 0, 0);

	if( !info->loop )
	{
		// 一次性定时器
		timer_delete(info->tt);
		callback.Dispose();
	}
}

void CTimer::clean()
{
	if( m_instance )
	{
		delete m_instance;
		m_instance = NULL;
	}
}
