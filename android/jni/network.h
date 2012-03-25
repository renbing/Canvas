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
#include <curl/curl.h>

#include <vector>
#include <string>
#include <map>

using std::string;
using std::vector;
using std::map;

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
		CURL *m_curl;

		DOWNLOAD_STATE m_state;

		DownloadCallback m_callback;
		void *m_callbackArg;
		bool m_callbackImmediately;
		
		bool m_async;
		string m_url;
		bool m_post;
		string m_postData;
		struct curl_slist *m_headers;
	
	private:
		void init();

	public:
		void didReceiveResponse(int status);
		void didReceiveData(void *data, size_t len);
		void didFinishLoading();
		void performDownload();

	public:
		Downloader();
		Downloader(const string *url, bool post = false, const string *postData = NULL, bool async = false, bool callbackImmediately = true, 
					DownloadCallback callback = NULL, void *callbackArg = NULL, const map<string, string> *headers = NULL);
		~Downloader();

		void downloadURL(const string *url, bool post = false , const string *postData = NULL, bool async = false, bool callbackImmediately = true, 
					DownloadCallback callback = NULL, void *callbackArg = NULL, const map<string, string> *headers = NULL);
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
		void downloadURL(const string *url, bool post = false, const string *postData = NULL, DownloadCallback callback = NULL, 
						void *callbackArg = NULL, const map<string, string> *headers = NULL);
};
