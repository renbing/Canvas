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

typedef struct
{
	string url;
	string post;
	string headers;
	Downloader *downloader;
}DownloadArgument;

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

	DownloadArgument *downloadArg = (DownloadArgument *)arg;
	Downloader *downloader = downloadArg->downloader;

	CURL *curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
	curl_easy_setopt(curl, CURLOPT_URL, downloadArg->url.c_str());

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

	delete downloadArg;

	return NULL;
}

Downloader::Downloader()
{
	m_status = -1;
	m_bufSize = 1024 * 100;
	m_buf = malloc(m_bufSize);
	m_recievedLen = 0;
	m_complete = false;

	m_callback = NULL;
	m_callbackArg = NULL;
	m_callbackImmediately = true;
}

Downloader::~Downloader()
{
	free(m_buf);
}

void Downloader::downloadURL(const string *url, bool async, bool callbackImmediately, DownloadCallback callback, void *callbackArg, const string *post, const string *headers)
{
	if( !url || url->empty() )
	{
		didFinishLoading();
		return;
	}

	m_callback = callback;
	m_callbackArg = callbackArg;
	m_callbackImmediately = callbackImmediately;

	DownloadArgument *arg = new DownloadArgument();
	arg->url = *url;
	if( post )
	{
		arg->post = *post;
	}
	if( headers )
	{
		arg->headers = *headers;
	}
	arg->downloader = this;

	if( async )
	{
		pthread_t tid;
		pthread_create(&tid, NULL, download, arg);
	}
	else
	{
		download(arg);
	}
}

void Downloader::didReceiveData(void *data, size_t len)
{
	// 多一个字节来处理string结尾字符
	if( m_recievedLen + len  >= m_bufSize )
	{
		// 超出缓冲区大小,增加缓冲区到2倍大小,如果缓冲区>1M,则只增加1M,减少内存浪费
		size_t newBufSize = m_bufSize * 2;
		if( m_bufSize > 1024 * 1024 )
		{
			newBufSize = m_bufSize + 1024 * 1024;
		}
		void *newBuf = realloc(m_buf, newBufSize);
		//memcpy(newBuf, m_buf, m_recievedLen);

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
	//LOG("download finish: %d", m_status);
	m_complete = true;
	if( m_callbackImmediately )
	{
		invokeCallback();
	}
}

void Downloader::invokeCallback()
{
	if( m_callback )
	{
		m_callback(this, m_callbackArg);
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

bool Downloader::isComplete()
{
	return m_complete;
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
	Downloader *downloader = new Downloader();
	downloader->downloadURL(url, true, false, callback, callbackArg, post, headers);
	m_downloaders.push_back(downloader);
}

void AsyncDownloadQueue::checkComplete()
{
	for( int i=0; i<m_downloaders.size(); )
	{
		Downloader *downloader = m_downloaders[i];
		if( downloader && downloader->isComplete() )
		{
			downloader->invokeCallback();	
			m_downloaders.erase(m_downloaders.begin()+i);
		}
		else
		{
			i++;
		}
	}
}
