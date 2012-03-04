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

#include <stdlib.h>
#include <vector>
#include <string>

using namespace std;

class Downloader;

typedef void (*DownloadCallback)(Downloader *, void *);

class Downloader
{
	private:
		void *m_buf;
		size_t m_bufSize;
		size_t m_recievedLen;
		int m_status;
		bool m_complete;

		DownloadCallback m_callback;
		void *m_callbackArg;
		bool m_callbackImmediately;
	
	public:
		void didReceiveResponse(int status);
		void didReceiveData(void *data, size_t len);
		void didFinishLoading();

	public:
		Downloader();
		~Downloader();

		void downloadURL(const string *url, bool async = false, bool callbackImmediately = true, DownloadCallback callback = NULL, 
						void *callbackArg = NULL, const string *post = NULL, const string *headers = NULL);
		const void * receivedData();
		size_t receivedDataLength();
		int status();
		bool isComplete();
		void invokeCallback();
};

class AsyncDownloadQueue
{
	private:
		static AsyncDownloadQueue *m_instance;
		vector<Downloader *> m_downloaders;			
	
	private:
		AsyncDownloadQueue();
	
	public:
		static AsyncDownloadQueue * getInstance();
		void checkComplete();	
		void downloadURL(const string *url, DownloadCallback callback = NULL, 
						void *callbackArg = NULL, const string *post = NULL, const string *headers = NULL);
};
