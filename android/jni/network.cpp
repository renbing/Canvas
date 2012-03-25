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
	downloader->performDownload();

	return NULL;
}

Downloader::~Downloader()
{
	if( m_headers != NULL )
	{
		curl_slist_free_all(m_headers);
	}

	curl_easy_cleanup(m_curl);
	free(m_buf);
}

Downloader::Downloader()
{
	init();
}

Downloader::Downloader(const string *url, bool post, const string *postData, bool async, bool callbackImmediately, DownloadCallback callback, 
							void *callbackArg, const map<string, string> *headers)
{
	init();

	downloadURL(url, post, postData, async, callbackImmediately, callback, callbackArg, headers);

}

void Downloader::init()
{
	m_status = -1;
	m_bufSize = 1024 * 100;
	m_buf = malloc(m_bufSize);
	m_recievedLen = 0;
	m_state = EMPTY;
	
	m_post = false;
	m_async = false;
	m_callback = NULL;
	m_callbackArg = NULL;
	m_callbackImmediately = true;

	m_headers = NULL;
	m_curl = curl_easy_init();
}

void Downloader::downloadURL(const string *url, bool post, const string *postData, bool async, bool callbackImmediately, DownloadCallback callback, 
							void *callbackArg, const map<string, string> *headers)
{
	if( m_state != EMPTY )
	{
		return;
	}

	m_post = post;
	m_async = async;
	m_callback = callback;
	m_callbackArg = callbackArg;
	m_callbackImmediately = callbackImmediately;

	if( url )
	{
		m_url = *url;
	}
	if( postData )
	{
		m_postData = *postData;
	}
	if( headers )
	{
		for( map<string,string>::const_iterator it=headers->begin(); it != headers->end(); it++ )
		{
			string header = it->first + ": " + it->second;
			m_headers = curl_slist_append(m_headers, header.c_str());
		}
	}

	m_state = PREPARED;
}

bool Downloader::start()
{
	if( m_state != PREPARED )
	{
		return false;
	}

	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writer);
	curl_easy_setopt(m_curl, CURLOPT_URL, m_url.c_str());

	//curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L);
	//curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);

	curl_easy_setopt(m_curl, CURLOPT_FORBID_REUSE, 1);
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
	
	if( m_post )
	{
		curl_easy_setopt(m_curl,CURLOPT_POST, 1);
		curl_easy_setopt(m_curl,CURLOPT_POSTFIELDS, m_postData.c_str());
	}
	
	curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headers);

	if( m_async )
	{
		pthread_t tid;
		pthread_create(&tid, NULL, download, (void *)this);
		pthread_detach(tid);
	}
	else
	{
		download( (void *)this );
	}

	m_state = RUNNING;

	return true;
}

void Downloader::performDownload()
{
	long repCode = -1;
	CURLcode res = CURLE_OK;

	res = curl_easy_perform(m_curl);
	if ( CURLE_OK == res )
	{
		curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &repCode);

		//char buf[8];
		//curl_easy_getinfo(m_curl, CURLINFO_CONTENT_TYPE, &buf);
	}

	didReceiveResponse(repCode);
	didFinishLoading();
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

void AsyncDownloadQueue::downloadURL(const string *url, bool post, const string *postData, DownloadCallback callback, 
									void *callbackArg, const map<string, string> *headers)
{
	Downloader *downloader = new Downloader(url, post, postData, true, false, callback, callbackArg, headers);
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
