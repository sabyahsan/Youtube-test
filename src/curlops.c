/*
 * curlops.cc
 *
 *  Created on: Feb 21, 2014
 *      Author: sahsan
 */

#include "curlops.h"
#include "helper.h"
#include "metrics.h"
#include "mm_parser.h"
#include "youtube-dl.h"
#include "coro.h"
#include "attributes.h"
#include <sys/select.h>

//#define SAVEFILE

extern metrics metric;
extern bool Inst;
extern int maxtime;
extern enum IPv ip_version;

#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         6000
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     1

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *userdata) {
	struct myprogress *p = (struct myprogress *)userdata;
	size_t len =size*nmemb;

	p->curl_buffer = (uint8_t*)ptr;
	p->bytes_avail = len;
	/* Tell the coroutines there is new data */
	if(p->stream == STREAM_VIDEO) {
		if(!p->init) {
			p->init = true;

			coro_stack_alloc(&p->parser_stack, 0);
			coro_create(&p->parser_coro, mm_parser, p, p->parser_stack.sptr, p->parser_stack.ssze);

			coro_transfer(&corou_main, &p->parser_coro);
		}

		coro_transfer(&corou_main, &p->parser_coro);
	} else if(p->stream == STREAM_AUDIO) {
		if(!p->init) {
			p->init = true;

			coro_stack_alloc(&p->parser_stack, 0);
			coro_create(&p->parser_coro, mm_parser, p, p->parser_stack.sptr, p->parser_stack.ssze);

			coro_transfer(&corou_main, &p->parser_coro);
		}

		coro_transfer(&corou_main, &p->parser_coro);
	}

	return len;
}

static int xferinfo(void *p,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t UNUSED(ultotal), curl_off_t UNUSED(ulnow))
{
  struct myprogress *myp = (struct myprogress *)p;
  CURL *curl = myp->curl;
  double totaltime, starttime, curtime;

  curl_easy_getinfo(curl, CURLINFO_STARTTRANSFER_TIME, &starttime);
  if(starttime == 0) {
	  return 0;
  }
  curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &totaltime);
  curtime = totaltime - starttime;

  /* if verbose is set then print the status  */
  if(Inst)
  {
	  if((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL)
	  {
		  printf("YOUTUBEINTERIM;%ld;%ld;", (long)gettimeshort(), (long)metric.htime);
		  printf("%.0f;", curtime*1000*1000);
		  printf("%"PRIu64";", metric.TSnow * 1000);
		  if(metric.numofstreams > 1)
		  {
			  if(myp->stream==STREAM_AUDIO)
				  printf("AUDIO;");
			  else if(myp->stream==STREAM_VIDEO)
				  printf("VIDEO;");
		  }
		  else
			  printf("ALL;");
		  printf("%"PRIu64";", metric.TSlist[myp->stream] * 1000);
		  printf("%ld;", (long)dlnow);
		  printf("%ld;", (long)dltotal);
		  printf("%.0f;", dlnow/curtime);
		  printf("%.0f;", (dlnow-myp->lastdlbytes)/(curtime-myp->lastruntime));

		  printf("%d;", metric.numofstalls);
		  printf("%.0f;", (metric.numofstalls>0 ? (metric.totalstalltime/metric.numofstalls) : 0)); // av stall duration
		  printf("%.0f\n", metric.totalstalltime);
		  myp->lastdlbytes = dlnow;
		  myp->lastruntime = curtime;
	  }
  }

  return 0;
}

/* for libcurl older than 7.32.0 (CURLOPT_PROGRESSFUNCTION) */
static int older_progress(void *p,
                          double dltotal, double dlnow,
                          double ultotal, double ulnow)
{
  return xferinfo(p,
                  (curl_off_t)dltotal,
                  (curl_off_t)dlnow,
                  (curl_off_t)ultotal,
                  (curl_off_t)ulnow);
}

static void my_curl_easy_returnhandler(CURLcode eret, int i)
{
	if(eret != CURLE_OK)
		fprintf(stderr, "curl_easy_setopt() failed for stream %d with %d: %s\n",i, eret, curl_easy_strerror(eret));
}

static int my_curl_cleanup(struct myprogress * prog, CURLM * multi_handle, CURL *http_handle[], int num, int errorcode)
{
	coro_destroy(&prog[0].parser_coro);
	coro_stack_free(&prog[0].parser_stack);
	if(metric.numofstreams > 1) {
		coro_destroy(&prog[1].parser_coro);
		coro_stack_free(&prog[1].parser_stack);
	}

	metric.etime = gettimelong();
	long http_code;
	for(int j =0; j<num; j++)
	{
	  	metric.etime= gettimelong();
	  	char * remoteip;
	  	curl_easy_getinfo (http_handle[j], CURLINFO_PRIMARY_IP, &remoteip);
	  	strcpy(metric.cdnip[j], remoteip);
	  	if(errorcode==ITWORKED)
	  	{
	  	    if( curl_easy_getinfo(http_handle[j], CURLINFO_SIZE_DOWNLOAD, &metric.totalbytes[j])!= CURLE_OK)
	  	    	metric.totalbytes[j]=-1;

	  	    /* Get download time */
	  	    if( curl_easy_getinfo(http_handle[j], CURLINFO_TOTAL_TIME, &metric.downloadtime[j])!= CURLE_OK)
	  	    	metric.downloadtime[j]=-1;
	  	    double starttransfer;
	  	    if( curl_easy_getinfo(http_handle[j], CURLINFO_STARTTRANSFER_TIME, &starttransfer)!= CURLE_OK)
	  	    	metric.downloadtime[j]=-1;

	  	    if(metric.downloadtime[j] != -1)
	  	    {
	  	    	metric.downloadtime[j] -= starttransfer;
	  	    }

	  	    /* Get download rate */
	  	    if(metric.downloadtime[j] != -1 && metric.totalbytes[j] != -1)
	  	    {
	  	    	metric.downloadrate[j] = metric.totalbytes[j] / metric.downloadtime[j];
	  	    }

	  	    /* Get connection time */
	  	    double lookup_time;
	  	    if( curl_easy_getinfo(http_handle[j], CURLINFO_CONNECT_TIME, &metric.connectiontime[j])!= CURLE_OK)
	  	    	metric.connectiontime[j]=-1;
	  	    else {
	  	    	if( curl_easy_getinfo(http_handle[j], CURLINFO_NAMELOOKUP_TIME, &lookup_time)!= CURLE_OK) {
	  	    		metric.connectiontime[j]=-1;
	  	    	} else {
	  	    		metric.connectiontime[j] -= lookup_time;
	  	    	}
	  	    }

	  	    if( curl_easy_getinfo (http_handle[j], CURLINFO_RESPONSE_CODE, &http_code)== CURLE_OK)
	  	    {
	  	    	if(http_code==200)
	  	    	{
	  	    		if(metric.Tmin<0)
	  	    			checkstall(true);
	  	    	}
	  	    	else if(http_code==0) {
	  	    		metric.errorcode = CURLERROR;
	  	    	}
	  	    	else
	  	    		metric.errorcode = http_code;
	  	    }
	  	    else
	  	    	metric.errorcode = CODETROUBLE;
	  	}
	  	curl_easy_cleanup(http_handle[j]);
	}
	curl_multi_cleanup(multi_handle);
	free(prog);
	if(metric.errorcode==0 || metric.errorcode==MAXTESTRUNTIME)
		metric.errorcode=errorcode;
	return metric.errorcode;
}

int set_ip_version(CURL *curl, enum IPv ip_version) {
	int ret = 0;

	CURLcode res;
	switch(ip_version) {
	case IPv4:
		res = curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
		if(res != CURLE_OK) {
			ret = -1;
		}
		break;
	case IPv6:
		res = curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6);
		if(res != CURLE_OK) {
			ret = -1;
		}
		break;
	case IPvSYSTEM:
		break;
	default:
		ret = -2;
	}

	return ret;
}

long getfilesize(const char url[])
{
	long ret;

	CURL *curl_handle = curl_easy_init();
	if(curl_handle == NULL) {
		ret = -1;
		goto out;
	}

	CURLcode error = 0;
	/* if redirected, tell libcurl to follow redirection */
	error |= curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);
	/* timeout if no response is received in 1 minute */
	error |= curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 60);
	/* just get the header for the file size */
	error |= curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1);
	error |= curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	error |= curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);

	if(error) {
		ret = -2;
		goto out;
	}

	/* request to download the file */
	CURLcode c = curl_easy_perform(curl_handle);
	if(c != CURLE_OK) {
		fprintf(stderr, "curl failed with %d: %s %s\n",c, curl_easy_strerror(c), url);
		ret = -3;
		goto out;
	}

	long http_code;
	error = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
	if(http_code != 200 || error != CURLE_OK) {
		ret = -4;
		goto out;
	}

	double content_length;
	error = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);
	if(error != CURLE_OK) {
		ret = -5;
		goto out;
	}

	ret = content_length;

out:
	curl_easy_cleanup(curl_handle);

	return ret;
}

int downloadfiles(videourl url [] )
{
	CURLMcode mret;

	CURL *http_handle[NUMOFSTREAMS];
	CURLM *multi_handle;
	int still_running; /* keep number of running handles */
	/* init a multi stack */
	multi_handle = curl_multi_init();
	if(multi_handle==NULL)
	{
		fprintf(stderr, "curl_multi_init() failed and returned NULL\n");
		return CURLERROR;
	}
	struct myprogress * prog= malloc(sizeof(struct myprogress [metric.numofstreams]));
	metric.stime = gettimelong();
	for(int i =0; i<metric.numofstreams; i++)
	{
		http_handle[i] = curl_easy_init();
		if(http_handle[i] == NULL)
		{
			fprintf(stderr, "curl_easy_init() failed and returned NULL\n");
			return my_curl_cleanup(prog, multi_handle, http_handle, i, CURLERROR);
		}

		set_ip_version(http_handle[i], ip_version);
		/* set options */
		my_curl_easy_returnhandler(curl_easy_setopt(http_handle[i], CURLOPT_URL, url[i].url),i);
		/* if redirected, tell libcurl to follow redirection */
		my_curl_easy_returnhandler(curl_easy_setopt(http_handle[i] , CURLOPT_FOLLOWLOCATION, 1L),i);
		/* if received 302, follow location  */
		my_curl_easy_returnhandler(curl_easy_setopt(http_handle[i], CURLOPT_POSTREDIR, 1L),i);	/* if redirected, tell libcurl to follow redirection */
		/* timeout if transaction is not completed in 5 and a half minutes */
		my_curl_easy_returnhandler(curl_easy_setopt(http_handle[i] , CURLOPT_TIMEOUT, 330),i);
		my_curl_easy_returnhandler(curl_easy_setopt(http_handle[i] , CURLOPT_REFERER, metric.link),i);
		my_curl_easy_returnhandler(curl_easy_setopt(http_handle[i], CURLOPT_SSL_VERIFYPEER, 0),i);

		/*setting the progress function*/
		prog[i].lastdlbytes = 0;
		prog[i].lastruntime = 0;
		prog[i].curl = http_handle[i];
		prog[i].stream = i;
		prog[i].init = false;
		my_curl_easy_returnhandler(curl_easy_setopt(http_handle[i], CURLOPT_NOPROGRESS, 0L),i);
		my_curl_easy_returnhandler(curl_easy_setopt(http_handle[i], CURLOPT_PROGRESSFUNCTION, older_progress),i);
		my_curl_easy_returnhandler(curl_easy_setopt(http_handle[i], CURLOPT_PROGRESSDATA, &prog[i]),i);
		my_curl_easy_returnhandler(curl_easy_setopt(http_handle[i], CURLOPT_WRITEFUNCTION, write_data),i);
		my_curl_easy_returnhandler(curl_easy_setopt(http_handle[i], CURLOPT_WRITEDATA, &prog[i]),i);

		/* add the individual transfers */
		mret = curl_multi_add_handle(multi_handle, http_handle[i]);
		if(mret != CURLM_OK)
		{
			fprintf(stderr, "curl_multi_add_handle() failed with %d: %s\n",mret, curl_multi_strerror(mret));
			return my_curl_cleanup(prog, multi_handle, http_handle, i,CURLERROR);
		}
	}

	/* we start some action by calling perform right away */
	mret = curl_multi_perform(multi_handle, &still_running);
	while (mret ==CURLM_CALL_MULTI_PERFORM)
		mret = curl_multi_perform(multi_handle, &still_running);

	if(mret != CURLM_OK)
	{
		fprintf(stderr, "curl_multi_perform() failed with %d: %s\n", mret, curl_multi_strerror(mret));
		return my_curl_cleanup(prog, multi_handle, http_handle, metric.numofstreams, CURLERROR);
	}

	do {
		struct timeval timeout;
		int rc; /* select() return code */

		fd_set fdread;
		fd_set fdwrite;
		fd_set fdexcep;
		int maxfd = -1;

		long curl_timeo = -1;

		FD_ZERO(&fdread);
		FD_ZERO(&fdwrite);
		FD_ZERO(&fdexcep);

		/* set a suitable timeout to play around with */
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		mret=curl_multi_timeout(multi_handle, &curl_timeo);
		if(mret==CURLM_OK)
		{
			if(curl_timeo >= 0) {
			  timeout.tv_sec = curl_timeo / 1000;
			  if(timeout.tv_sec > 1)
				timeout.tv_sec = 1;
			  else
				timeout.tv_usec = (curl_timeo % 1000) * 1000;
			}

			/* get file descriptors from the transfers */
			mret = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);
		}
		if(mret!=CURLM_OK)
		{
			  fprintf(stderr, "curl_multi_timeout()/curl_multi_fdset failed with %d: %s\n", mret, curl_multi_strerror(mret));
				return my_curl_cleanup(prog, multi_handle, http_handle, metric.numofstreams, CURLERROR);
		}
		/* In a real-world program you OF COURSE check the return code of the
		   function calls.  On success, the value of maxfd is guaranteed to be
		   greater or equal than -1.  We call select(maxfd + 1, ...), specially in
		   case of (maxfd == -1), we call select(0, ...), which is basically equal
		   to sleep. */

		rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);

		switch(rc) {
		case -1:
		  fprintf(stderr, "select error occured: %d , %s\n", errno, strerror(errno));
		  return my_curl_cleanup(prog, multi_handle, http_handle, metric.numofstreams, CURLERROR);

		  break;
		case 0:
		default:
		  /* timeout or readable/writable sockets */
			mret = curl_multi_perform(multi_handle, &still_running);
			while (mret ==CURLM_CALL_MULTI_PERFORM)
				mret = curl_multi_perform(multi_handle, &still_running);

			if(mret != CURLM_OK)
			{
				fprintf(stderr, "curl_multi_perform() failed with %d: %s\n", mret, curl_multi_strerror(mret));
				return my_curl_cleanup(prog, multi_handle, http_handle, metric.numofstreams, CURLERROR);
			}
		  break;
		}

		if(metric.initialprebuftime >= 0) {
			long long now = gettimelong();
			long long playback_start_time = metric.stime + metric.initialprebuftime;
			if((now - playback_start_time) / 1000000 > maxtime)
			/*the test has been running for too long
			terminate the session. */
			{
				metric.errorcode = MAXTESTRUNTIME;
			}
		}

		checkstall(false);

		if(metric.errorcode) {
			break;
		}
	} while(still_running);

	return my_curl_cleanup(prog, multi_handle, http_handle, metric.numofstreams, ITWORKED);

}
