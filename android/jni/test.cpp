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

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <string.h>
#include <pthread.h>

#include <png.h>
#include <zip.h>
#include <curl/curl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include "ifaddrs.h"

#include "global.h"
#include "point.h"

extern zip *apkArchive;

static Handle<Value> alert( const Arguments& args )
{
	HandleScope handleScope;

	if( args.Length() > 0 )
	{
		const char *content = *(String::Utf8Value(args[0]));
		LOG("alert: %s", content);
	}

	return Undefined();
}

// 单元模块测试函数
void drawTest()
{
	glClear(GL_COLOR_BUFFER_BIT);

	const GLfloat vertices[] = {
		0, 100,
		100, 100,
		0, 0,
		100, 0
	};
	
	const GLubyte colors[] = {
		255,255,0,255,
		255,0,255,255,
		0,255,255,255,
		255,255,255,255,
	};

	const GLfloat textureCoords[] = {
		0.0, 0.0,
		1.0, 0.0,
		0.0, 1.0,
		1.0, 1.0
	};

	//glDisable(GL_BLEND);

	//glEnable(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, texture);

	//glEnable(GL_BLEND);
	//glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	//glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
	//glTexCoordPointer(2, GL_FLOAT, 0, textureCoords);
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void jsTest()
{
	HandleScope handleScope;
	Handle<ObjectTemplate> globalTpl = ObjectTemplate::New();
	
	// 注册自定义C++函数以及C++类
	globalTpl->Set(String::New("alert"), FunctionTemplate::New(alert));
	globalTpl->Set(String::New("Point"), Point::exportJS());

	{
		Persistent<Context> context = Context::New(0, globalTpl);
		Context::Scope contextScope(context);
		{
			Handle<String> source = String::New("var p = new Point(); p.x=100; alert(p.x); p.talk(); p.src='test js string'; alert(p.src);alert('alert'); 'Hello' + ', World!'");
			Handle<Script> script = Script::Compile(source);
			Handle<Value> result = script->Run();

			String::AsciiValue ascii(result);
			LOG("js: %s", *ascii);
		}

		context.Dispose();
	}
}

void zipTest()
{
	struct zip_stat zstat;
	zip_stat_init(&zstat);

	int numFiles = zip_get_num_files(apkArchive);
	for(int i=0; i<numFiles; i++)
	{
		const char *zipName = zip_get_name(apkArchive, i, 0);
		if( zipName != NULL )
		{
			zip_stat(apkArchive, zipName, 0, &zstat);	
			LOG("apk zip file:%s size:%d", zstat.name, zstat.size);
		}
	}

}

static void testTimerHandler(sigval_t v)
{
	LOG("signal %d captured", v.sival_int);
}

int testTimerCreate(int msec, bool loop)
{
	timer_t   tid;
	
	// 系统自增长定时器唯一ID
	static int timerid = 0;
	timerid ++;

	struct sigevent se; 
	struct itimerspec ts, ots; 

	memset(&se, 0, sizeof(se)); 

	se.sigev_notify = SIGEV_THREAD;
	se.sigev_notify_function = testTimerHandler;
	se.sigev_value.sival_int = timerid; 

	if( timer_create(CLOCK_REALTIME, &se, &tid) < 0 ) 
	{ 
		LOG("create timer fail");
		return   -1; 
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

	if( timer_settime(tid, TIMER_ABSTIME, &ts, &ots) < 0 )
	{
		LOG("start timer fail");
		return   -1;
	}

	return timerid;
}

// CURL 下载数据写入
static int writer(char *data, size_t size, size_t nmemb, char *writerData)
{
	if (writerData == NULL) return 0;
	//writerData->append(data, sizes);
	strcat(writerData, data);

	return size * nmemb;
}

static int writer2(char *data, size_t size, size_t nmemb, char *writerData)
{
	if (writerData == NULL) return 0;
	//writerData->append(data, sizes);
	strcat(writerData, data);

	return size * nmemb;
}

static void * download(void *url)
{
	LOG("download\n");
	CURL *curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
	curl_easy_setopt(curl, CURLOPT_URL, (char *)url);

	long repCode = -1;
	CURLcode res = CURLE_OK;

	char buf[102400];

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);
	res = curl_easy_perform(curl);
	if ( CURLE_OK == res )
	{
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &repCode);

		//char buf[8];
		//curl_easy_getinfo(m_curl,CURLINFO_CONTENT_TYPE,&buf);
	}

	LOG("curl %d: %s\n", repCode, buf);

	curl_easy_cleanup(curl);
}

void curlTest()
{
	download((void *)"http://www.baidu.com");
	download((void *)"http://10.0.2.9");
	//pthread_t tid;
	//pthread_create(&tid, NULL, download, (void *)"http://www.baidu.com");
}


void curlMutiTest()
{
	char buf1[102400];
	char buf2[102400];

	curl_global_init(CURL_GLOBAL_NOTHING);
	
	CURLM *multi_curl = curl_multi_init();

	CURL *curl1 = curl_easy_init();
	curl_easy_setopt(curl1, CURLOPT_WRITEFUNCTION, writer);
	curl_easy_setopt(curl1, CURLOPT_URL, "http://www.baidu.com");
	curl_easy_setopt(curl1, CURLOPT_WRITEDATA, buf1);

	CURL *curl2 = curl_easy_init();
	curl_easy_setopt(curl2, CURLOPT_WRITEFUNCTION, writer2);
	curl_easy_setopt(curl2, CURLOPT_URL, "http://www.google.com");
	curl_easy_setopt(curl2, CURLOPT_WRITEDATA, buf2);

	curl_multi_add_handle(multi_curl, curl1);
	curl_multi_add_handle(multi_curl, curl2);
	
	int running_handle_count;
	do{
		while( CURLM_CALL_MULTI_PERFORM == curl_multi_perform(multi_curl, &running_handle_count) )
		{
			
		}

		timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		int max_fd;
		fd_set fd_read;
		fd_set fd_write;
		fd_set fd_except;

		FD_ZERO(&fd_read);
		FD_ZERO(&fd_write);
		FD_ZERO(&fd_except);

		curl_multi_fdset(multi_curl, &fd_read, &fd_write, &fd_except, &max_fd);
		if( -1 == select(max_fd + 1, &fd_read, &fd_write, &fd_except, &tv) )
		{
			LOG("select error\n");
			break;
		}

	}while( running_handle_count );

	LOG("curl1: %s\n", buf1);
	LOG("curl2: %s\n", buf2);

	curl_easy_cleanup(curl1);
	curl_easy_cleanup(curl2);
	curl_multi_cleanup(multi_curl);
	curl_global_cleanup();
}

void socketTest()
{
	struct ifaddrs *ifap;
	if( getifaddrs(&ifap) != 0 )
	{
		LOG("get network interface error\n");
		exit(errno);
	}
	
	struct ifaddrs *p = ifap;
	for(; p; p=p->ifa_next )
	{
		struct sockaddr_in *addr;
		uint32_t ip;

		addr = (struct sockaddr_in *)(p->ifa_addr);
		if( !addr )
		{
			LOG("not ip");
			continue;
		}
		
		ip = ntohl(addr->sin_addr.s_addr);
		if( ip == 0 || ip == 2130706433) // 127.0.0.1
		{
			LOG("localhost ip");
			continue;
		}
		LOG("myIP: %s %u %i.%i.%i.%i\n", p->ifa_name, ip, (ip>>24)&0xFF, (ip>>16)&0xFF, (ip>>8)&0xFF, (ip>>0)&0xFF);

		addr = (struct sockaddr_in *)(p->ifa_netmask);
		if( !addr )
		{
			LOG("not netmask");
			continue;
		}
		
		ip = ntohl(addr->sin_addr.s_addr);
		if( ip == 0 || ip == 0xff000000) // 127.0.0.1
		{
			LOG("localhost netmask");
			continue;
		}
		
		LOG("broadcast IP: %u %i.%i.%i.%i\n", ip, (ip>>24)&0xFF, (ip>>16)&0xFF, (ip>>8)&0xFF, (ip>>0)&0xFF);
	}
	
	freeifaddrs(ifap);
}
