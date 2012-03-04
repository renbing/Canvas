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

#include <curl/curl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "network.h"
#include "global.h"

// CURL 下载数据写入
static int writer(void *data, size_t size, size_t nmemb, Downloader *downloader)
{
	
	//LOG("download writer\n");
	if( downloader )
	{
		downloader->didReceiveData(data, size*nmemb);
	}

	return size * nmemb;
}

// CURL 下载线程
static void * download(void *arg)
{
	//LOG("downloading...\n");

	Downloader *downloader = (Downloader *)arg;

	CURL *curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
	curl_easy_setopt(curl, CURLOPT_URL, downloader->url.c_str());

	//curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);

	long repCode = -1;
	CURLcode res = CURLE_OK;

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, downloader);
	res = curl_easy_perform(curl);
	if ( CURLE_OK == res )
	{
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &repCode);

		//char buf[8];
		//curl_easy_getinfo(m_curl,CURLINFO_CONTENT_TYPE,&buf);
	}

	curl_easy_cleanup(curl);
	downloader->didReceiveResponse(repCode);
	downloader->didFinishLoading();

	return NULL;
}

Downloader::~Downloader()
{
	free(m_buf);
}
Downloader::Downloader()
{
	m_status = -1;
	m_bufSize = 1024 * 100;
	m_buf = malloc(m_bufSize);
	m_recievedLen = 0;
	m_state = EMPTY;
	
	async = false;
	callback = NULL;
	callbackArg = NULL;
	callbackImmediately = true;
}

Downloader::Downloader(const string *url, bool async, bool callbackImmediately, DownloadCallback callback, void *callbackArg, const string *post, const string *headers)
{
	m_status = -1;
	m_bufSize = 1024 * 100;
	m_buf = malloc(m_bufSize);
	m_recievedLen = 0;
	m_state = PREPARED;
	
	this->async = async;
	this->callback = callback;
	this->callbackArg = callbackArg;
	this->callbackImmediately = callbackImmediately;

	if( url )
	{
		this->url = *url;
	}
	if( post )
	{
		this->post = *post;
	}
	if( headers )
	{
		this->headers = *headers;
	}
}

bool Downloader::start()
{
	if( m_state != PREPARED )
	{
		return false;
	}

	if( async )
	{
		pthread_t tid;
		pthread_create(&tid, NULL, download, (void *)this);
	}
	else
	{
		download( (void *)this );
	}

	m_state = RUNNING;

	return true;
}

void Downloader::didReceiveData(void *data, size_t len)
{
	// 多一个字节来处理string结尾字符
	if( m_recievedLen + len  >= m_bufSize )
	{
		// 超出缓冲区大小,增加缓冲区到2倍大小
		size_t newBufSize = m_bufSize * 2;
		void *newBuf = realloc(m_buf, newBufSize);

		m_buf = newBuf;
		m_bufSize = newBufSize;
	}
	
	memcpy((char *)m_buf+m_recievedLen, data, len);
	m_recievedLen += len;

	*((char *)m_buf+m_recievedLen) = 0;
}

void Downloader::didReceiveResponse(int status)
{
	m_status = status;
}

void Downloader::didFinishLoading()
{
	m_state = COMPLETE;

	if( callbackImmediately )
	{
		invokeCallback();
	}
}

void Downloader::invokeCallback()
{
	if( callback )
	{
		callback(this, callbackArg);
	}
}

const void * Downloader::receivedData()
{
	return m_buf;
}

size_t Downloader::receivedDataLength()
{
	return m_recievedLen;
}

int Downloader::status()
{
	return m_status;
}

DOWNLOAD_STATE Downloader::state()
{
	return m_state;
}

AsyncDownloadQueue * AsyncDownloadQueue::m_instance = NULL;

AsyncDownloadQueue::AsyncDownloadQueue()
{
}

AsyncDownloadQueue * AsyncDownloadQueue::getInstance()
{
	if( m_instance == NULL )
	{
		// 需要加锁
		if( m_instance == NULL )
		{
			m_instance = new AsyncDownloadQueue();
		}
	}

	return m_instance;
}

void AsyncDownloadQueue::downloadURL(const string *url, DownloadCallback callback, void *callbackArg, const string *post, const string *headers)
{
	Downloader *downloader = new Downloader(url, true, false, callback, callbackArg, post, headers);
	m_downloaders.push_back(downloader);
}

void AsyncDownloadQueue::checkComplete()
{
	int maxRuning = 10;
	for( int i=0; i<m_downloaders.size(); )
	{
		Downloader *downloader = m_downloaders[i];
		if( downloader && downloader->state() == COMPLETE )
		{
			downloader->invokeCallback();	
			m_downloaders.erase(m_downloaders.begin()+i);
		}
		else
		{
			if( downloader && downloader->state() == RUNNING )
			{
				maxRuning -= 1;
			}
			i++;
		}
	}
	
	if( maxRuning > 0 )
	{
		for( int i=0; i<m_downloaders.size() && maxRuning > 0; i++)
		{
			Downloader *downloader = m_downloaders[i];
			if( downloader && downloader->state() == PREPARED )
			{
				downloader->start();
				maxRuning -= 1;
			}
		}
	}
}
