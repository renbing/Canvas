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
typedef enum{ EMPTY, PREPARED, RUNNING, COMPLETE} DOWNLOAD_STATE;
class Downloader
{
	private:
		void *m_buf;
		size_t m_bufSize;
		size_t m_recievedLen;
		int m_status;

		DOWNLOAD_STATE m_state;

	public:
		DownloadCallback callback;
		void *callbackArg;
		bool callbackImmediately;
		
		bool async;
		string url;
		string post;
		string headers;

	public:
		void didReceiveResponse(int status);
		void didReceiveData(void *data, size_t len);
		void didFinishLoading();

	public:
		Downloader();
		Downloader(const string *url, bool async = false, bool callbackImmediately = true, DownloadCallback callback = NULL, 
						void *callbackArg = NULL, const string *post = NULL, const string *headers = NULL);
		~Downloader();
		
		bool start();
		const void * receivedData();
		size_t receivedDataLength();
		int status();
		DOWNLOAD_STATE state();
		void invokeCallback();
};

typedef struct
{
	string url;
	string post;
	string headers;
	DownloadCallback callback;
	void *callbackArg;

}AsyncDownloadItem;

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
