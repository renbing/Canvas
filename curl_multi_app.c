/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2011, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/ 
/* This is an example application source code using the multi interface. */ 
 
#include <stdio.h>
#include <string.h>
 
/* somewhat unix-specific */ 
#include <sys/time.h>
#include <unistd.h>
 
/* curl stuff */ 
#include <curl/curl.h>
 
/*
 * Download a HTTP file and upload an FTP file simultaneously.
 */ 
 
#define HANDLECOUNT 2    
#define HTTP_HANDLE 0    
#define FTP_HANDLE 1     

// CURL 下载数据写入
static int writer(void *data, size_t size, size_t nmemb, void *arg)
{
    printf("download writer %d\n", size * nmemb);

    return size * nmemb;
}
 
int main(void)
{
  CURL *handles[HANDLECOUNT];
  CURLM *multi_handle;
 
  int still_running; /* keep number of running handles */ 
  int i;
 
  CURLMsg *msg; /* for picking up messages with the transfer status */ 
  int msgs_left; /* how many messages are left */ 
 
  /* Allocate one CURL handle per transfer */ 
  for (i=0; i<HANDLECOUNT; i++)
      handles[i] = curl_easy_init();
 
  /* set the options (I left out a few, you'll get the point anyway) */ 
  curl_easy_setopt(handles[HTTP_HANDLE], CURLOPT_URL, "http://192.168.0.123/a.php");
   curl_easy_setopt(handles[HTTP_HANDLE], CURLOPT_WRITEFUNCTION, writer); 
   curl_easy_setopt(handles[HTTP_HANDLE], CURLOPT_TIMEOUT, 5);
//   curl_easy_setopt(handles[HTTP_HANDLE], CURLOPT_BUFFERSIZE, 10240);
 
  curl_easy_setopt(handles[FTP_HANDLE], CURLOPT_URL, "http://www.baidu.com");
   curl_easy_setopt(handles[FTP_HANDLE], CURLOPT_WRITEFUNCTION, writer); 
  //curl_easy_setopt(handles[FTP_HANDLE], CURLOPT_UPLOAD, 1L);
 
  /* init a multi stack */ 
  multi_handle = curl_multi_init();
 
  /* add the individual transfers */ 
  for (i=0; i<HANDLECOUNT; i++)
      curl_multi_add_handle(multi_handle, handles[i]);
 
  /* we start some action by calling perform right away */ 
  curl_multi_perform(multi_handle, &still_running);
 
struct timeval timeout;
    int rc; /* select() return code */ 
    /* set a suitable timeout to play around with */ 
    timeout.tv_sec = 0;
    timeout.tv_usec = 500;

  do {
 
    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd = -1;
 
    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);

    timeout.tv_sec = 0;
    timeout.tv_usec = 500;
 
    long curl_timeo = -1;
    curl_multi_timeout(multi_handle, &curl_timeo);
    if(curl_timeo >= 0) {
      timeout.tv_sec = curl_timeo / 1000;
      if(timeout.tv_sec > 1)
        timeout.tv_sec = 1;
      else
        timeout.tv_usec = (curl_timeo % 1000) * 1000;
    }
 
    /* get file descriptors from the transfers */ 
    curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);
 
    /* In a real-world program you OF COURSE check the return code of the
       function calls.  On success, the value of maxfd is guaranteed to be
       greater or equal than -1.  We call select(maxfd + 1, ...), specially in
       case of (maxfd == -1), we call select(0, ...), which is basically equal
       to sleep. */ 
 
    rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
 
    switch(rc) {
    case -1:
      /* select error */ 
      break;
    case 0: /* timeout */ 
    default: /* action */ 
      curl_multi_perform(multi_handle, &still_running);
      break;
    }
  /* See how the transfers went */ 
  while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
    if (msg->msg == CURLMSG_DONE) {
      int idx, found = 0;
 
      /* Find out which handle this message is about */ 
      for (idx=0; idx<HANDLECOUNT; idx++) {
        found = (msg->easy_handle == handles[idx]);
        if(found)
          break;
      }
      if( !found ) {
        continue;
      }
    long repCode = -1;
        curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &repCode);
 
      switch (idx) {
      case HTTP_HANDLE:
        printf("HTTP transfer completed with status %d code %d\n", msg->data.result, repCode);
        break;
      case FTP_HANDLE:
        printf("FTP transfer completed with status %d code %d\n", msg->data.result, repCode);
        break;
      }
    }
    curl_easy_cleanup(msg->easy_handle);
    curl_multi_remove_handle(multi_handle, msg->easy_handle);
  }
  } while(still_running);
 
 
  curl_multi_cleanup(multi_handle);
 
  return 0;
}
