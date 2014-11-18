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


static void my_curl_easy_returnhandler(CURLcode eret, int i)
{
	if(eret != CURLE_OK)
		fprintf(stderr, "curl_easy_setopt() failed for stream %d with %d: %s\n",i, eret, curl_easy_strerror(eret));
}

static int xferinfo(void *p,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t UNUSED(ultotal), curl_off_t UNUSED(ulnow))
{
  struct myprogress *myp = (struct myprogress *)p;
  CURL *curl = myp->curl;
  double totaltime, starttime, curtime, dlspeed;

  curl_easy_getinfo(curl, CURLINFO_STARTTRANSFER_TIME, &starttime);
  if(starttime == 0) {
          return 0;
  }
  curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &totaltime);
  curtime = totaltime - starttime;
  curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &dlspeed);
  /* if verbose is set then print the status  */
  if(Inst)
  {
          if(((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL) || (dlnow==dltotal && myp->lastdlbytes!=dlnow && dlnow!=0))
          {
		  long long timenow = gettimelong(); 
                  printf("YOUTUBEINTERIM;%ld;", (long)metric.htime/1000000);
                  printf("%ld;", (long)(timenow- metric.stime));
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
		  printf("%.0f;", dlspeed);
                  printf("%.0f;", (dlnow-myp->lastdlbytes)/(curtime-myp->lastruntime));
		  printf("%.0f;", (double)metric.totalbytes[myp->stream]/((double)(timenow-metric.stime)/1000000));
                  printf("%d;", metric.numofstalls);
                  printf("%.0f;", (metric.numofstalls>0 ? (metric.totalstalltime/metric.numofstalls) : 0)); // av stall duration
                  printf("%.0f\n", metric.totalstalltime);
                  myp->lastdlbytes = dlnow;
                  myp->lastruntime = curtime;
          }
/*	  else if (dlnow==dltotal)
	  	printf("They are equal: %.0f %.0f %.0f\n", dlnow, dltotal, myp->lastdlbytes); 
	else if( myp->lastdlbytes!=dlnow)
	  	printf("They are not equal: %.0f %.0f %.0f\n", dlnow, dltotal, myp->lastdlbytes); 
*/  }

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


static int update_curl_progress(struct myprogress * prog, CURL *http_handle[], int j)
{
        double starttransfer, downloadtime, totalbytes;
  	double lookup_time;
	/*Get total bytes*/
	if( curl_easy_getinfo(http_handle[j], CURLINFO_SIZE_DOWNLOAD, &totalbytes)!= CURLE_OK)
		metric.totalbytes[j]=-1;
	if(metric.totalbytes[j]!=-1)
		metric.totalbytes[j]+=totalbytes;
	/* Get download time */
        if( curl_easy_getinfo(http_handle[j], CURLINFO_TOTAL_TIME, &downloadtime)!= CURLE_OK)
                metric.downloadtime[j]=-1;
        if( curl_easy_getinfo(http_handle[j], CURLINFO_STARTTRANSFER_TIME, &starttransfer)!= CURLE_OK)
                metric.downloadtime[j]=-1;

        if(metric.downloadtime[j] != -1)
	{
                downloadtime-= starttransfer;
		metric.downloadtime[j]+=downloadtime;
	}
  	/* Get connection time and CDN IP - Only for the first connect */
	if(metric.connectiontime[j]==0)
	{
		if( curl_easy_getinfo(http_handle[j], CURLINFO_CONNECT_TIME, &metric.connectiontime[j])!= CURLE_OK)
		    metric.connectiontime[j]=-1;
		else {
			if( curl_easy_getinfo(http_handle[j], CURLINFO_NAMELOOKUP_TIME, &lookup_time)!= CURLE_OK) {
		  	    	metric.connectiontime[j]=-1;
		  	} else {
		  		metric.connectiontime[j] -= lookup_time;
		  	}
		}
	  	char * remoteip;
	  	curl_easy_getinfo (http_handle[j], CURLINFO_PRIMARY_IP, &remoteip);
	  	strcpy(metric.cdnip[j], remoteip);
	}
	return 1; 

}

static int my_curl_cleanup(struct myprogress * prog, CURLM * multi_handle, CURL *http_handle[], int num, int errorcode, videourl url [])
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
//		printf("metric printing %d\n", url[j].playing); fflush(stdout); 
//		exit(1); 
		if(metric.url[j].playing)
		    update_curl_progress(prog, http_handle, j);
	  	if(errorcode==ITWORKED)
	  	{
	  	    /* Calculate download rate */
	  	    if(metric.downloadtime[j] != -1 && metric.totalbytes[j] != -1)
	  	    	metric.downloadrate[j] = metric.totalbytes[j] / metric.downloadtime[j];
		    else
			metric.downloadrate[j]=-1; 

 		    if(metric.url[j].playing && metric.errorcode==0)
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
//	if(metric.errorcode==0 || metric.errorcode==MAXTESTRUNTIME)
	if(metric.errorcode==0)
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

/*Adapted from - http://curl.haxx.se/libcurl/c/multi-app.html*/
int initialize_curl_handle( CURL ** http_handle_ref, int i, videourl * url, struct myprogress * prog, CURLM *multi_handle)
{
	char range[50];
	char url_now[1500];
	strcpy(url_now, url->url);
	if(metric.playout_buffer_seconds>0)
	{
		sprintf(range, "&range=%ld-%ld",url->range0, url->range1);
		strcat(url_now,range);
	}
	CURL * http_handle;
	if(*http_handle_ref != NULL)
	{
		http_handle = *http_handle_ref;
		curl_multi_remove_handle(multi_handle,http_handle);
	}
	else
	{
		prog->stream = i;
                prog->init = false;
	}
	prog->lastdlbytes = 0;
	prog->lastruntime = 0;

	http_handle = curl_easy_init();
	if(http_handle ==NULL)
	{
		fprintf(stderr, "curl_easy_init() failed and returned NULL\n");
		return 0;
	}
	*http_handle_ref = http_handle;


	/* set options */

	my_curl_easy_returnhandler(curl_easy_setopt(http_handle, CURLOPT_URL, url_now),i);
	/* if redirected, tell libcurl to follow redirection */
	my_curl_easy_returnhandler(curl_easy_setopt(http_handle , CURLOPT_FOLLOWLOCATION, 1L),i);
	/* if received 302, follow location  */
	my_curl_easy_returnhandler(curl_easy_setopt(http_handle, CURLOPT_POSTREDIR, 1L),i);	/* if redirected, tell libcurl to follow redirection */

	/* timeout if transaction is not completed in 5 and a half minutes */
	my_curl_easy_returnhandler(curl_easy_setopt(http_handle , CURLOPT_TIMEOUT, 330),i);
	my_curl_easy_returnhandler(curl_easy_setopt(http_handle, CURLOPT_REFERER, metric.link),i);
	my_curl_easy_returnhandler(curl_easy_setopt(http_handle, CURLOPT_SSL_VERIFYPEER, 0),i);
	/*setting the progress function only when range is not given, otherwise interim results are published for each chunk only*/
	prog->curl = http_handle;
/*	if(metric.playout_buffer_seconds>0)
	{
                prog->lastdlbytes = 0;
                prog->lastruntime = 0;
	}
*/                my_curl_easy_returnhandler(curl_easy_setopt(http_handle, CURLOPT_NOPROGRESS, 0L),i);
                my_curl_easy_returnhandler(curl_easy_setopt(http_handle, CURLOPT_PROGRESSFUNCTION, older_progress),i);
                my_curl_easy_returnhandler(curl_easy_setopt(http_handle, CURLOPT_PROGRESSDATA, prog),i);

	my_curl_easy_returnhandler(curl_easy_setopt(http_handle, CURLOPT_WRITEFUNCTION, write_data),i);
	my_curl_easy_returnhandler(curl_easy_setopt(http_handle, CURLOPT_WRITEDATA, prog),i);


		/* add the individual transfers */
		CURLMcode mret = curl_multi_add_handle(multi_handle, http_handle);
		if(mret != CURLM_OK)
		{
			fprintf(stderr, "curl_multi_add_handle() failed with %d: %s\n",mret, curl_multi_strerror(mret));
			return 0;
		}
	return 1;

}

int downloadfiles(videourl url [] )
{
	CURLMcode mret;

	CURL *http_handle[NUMOFSTREAMS];
	memzero(http_handle,sizeof(CURL *)*NUMOFSTREAMS);
	CURLM *multi_handle;
	CURLMsg *msg; /* for picking up messages with the transfer status */
	int msgs_left; /* how many messages are left */
	int still_running; /* keep number of running handles */
	int run = metric.numofstreams;
	/* init a multi stack */
	multi_handle = curl_multi_init();
	if(multi_handle==NULL)
	{
		fprintf(stderr, "curl_multi_init() failed and returned NULL\n");
		return CURLERROR;
	}
	struct myprogress * prog= malloc(sizeof(struct myprogress [metric.numofstreams]));
	if(metric.stime==0)
	{
		metric.stime = gettimelong();
		metric.startup = metric.stime - metric.htime; 
	}
	else 
		metric.stime = gettimelong();
	while(run)
	{
		run=0; 
		/*Previous transfers have finished, figure out if we have enough space in the buffer to download 
		  more or should we wait while the buffer empties */
		struct timeval playoutbuffer_len;
		/*Get the length of the current buffer*/
		playoutbuffer_len = get_curr_playoutbuf_len();
		/*Adjust timetowait based on the length of the playout buffer, we don't care about usecs*/
		if(playoutbuffer_len.tv_sec>metric.playout_buffer_seconds)
		{
			playoutbuffer_len.tv_sec = playoutbuffer_len.tv_sec-metric.playout_buffer_seconds;
#ifdef DEBUG
			printf("Time to wait: %d seconds. Sleeping at %ld....", playoutbuffer_len.tv_sec, gettimeshort());
#endif
			select(0, NULL, NULL, NULL, &playoutbuffer_len);
#ifdef DEBUG
			printf("Awake %ld\n", gettimeshort());
#endif
		}

		for(int i =0; i<metric.numofstreams; i++)
		{
			url[i].range0=url[i].range1;
			/*If range1 is greater than zero, this is not the first fetch*/
			if(url[i].range1>0)
				++url[i].range0;
			/*First fetch, set playing to 1 for the stream*/
			else
				url[i].playing = 1;
			if(!url[i].playing)
			{
				/*the stream is not playing, move on*/
				continue;
			}
			else
				++run;
			/*Time to download more, check separately for each stream to decide how much to download. */ 
			playoutbuffer_len = get_curr_playoutbuf_len_forstream(i);
			if(playoutbuffer_len.tv_sec>metric.playout_buffer_seconds)
				continue;
			if((metric.playout_buffer_seconds-playoutbuffer_len.tv_sec)>=LEN_CHUNK_MINIMUM)
				url[i].range1+=url[i].bitrate*LEN_CHUNK_MINIMUM;
			else
				url[i].range1+=url[i].bitrate*LEN_CHUNK_FETCH;

#ifdef DEBUG
			printf("Getting next chunk for stream %d with range : %ld - %ld\n", i, url[i].range0, url[i].range1);
#endif
			if(!initialize_curl_handle(http_handle+i, i,url+i,prog+i, multi_handle))
				my_curl_cleanup(prog, multi_handle, http_handle, i, CURLERROR,url);
		}
		if(run==0)
			break; 
		/* we start some action by calling perform right away */
		mret = curl_multi_perform(multi_handle, &still_running);
		while (mret ==CURLM_CALL_MULTI_PERFORM)
			mret = curl_multi_perform(multi_handle, &still_running);

		if(mret != CURLM_OK)
		{
			fprintf(stderr, "curl_multi_perform() failed with %d: %s\n", mret, curl_multi_strerror(mret));
			return my_curl_cleanup(prog, multi_handle, http_handle, metric.numofstreams, CURLERROR, url);
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
				return my_curl_cleanup(prog, multi_handle, http_handle, metric.numofstreams, CURLERROR,url);
			}

			rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);

			switch(rc) {
			case -1:
			  fprintf(stderr, "select error occured: %d , %s\n", errno, strerror(errno));
			  return my_curl_cleanup(prog, multi_handle, http_handle, metric.numofstreams, CURLERROR,url);

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
					return my_curl_cleanup(prog, multi_handle, http_handle, metric.numofstreams, CURLERROR,url);
				}
			  break;
			}



			while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) 
			{
				if (msg->msg == CURLMSG_DONE) {
				  int idx, found = 0;

				  /* Find out which handle this message is about */
				  double bytesnow=0, dlspeed=0; 
				  for (idx=0; idx<metric.numofstreams; idx++) {
					found = (msg->easy_handle == http_handle[idx]);
					if(found)
					{
						if(url[idx].playing )
							update_curl_progress(prog, http_handle, idx);
						if( curl_easy_getinfo(http_handle[idx], CURLINFO_SIZE_DOWNLOAD, &bytesnow)!= CURLE_OK)
						{
							url[idx].playing = 0;
							metric.errorcode=CURLERROR_GETINFO;
						}
						else
						{
							if(bytesnow<url[idx].range1-url[idx].range0)
							{
#ifdef DEBUG
								printf("Full HTTP transfer completed for stream %d  with status %d\n", idx, msg->data.result);fflush(stdout); 
#endif
								url[idx].playing=0;
							}
						}
						break;
					}
				  }
				}
			}
			if(metric.initialprebuftime >= 0) {
				long long now = gettimelong();
				long long playback_start_time = metric.stime + metric.initialprebuftime;
				if((now - playback_start_time) / 1000000 > maxtime)
				/*the test has been running for too long
				terminate the session. */
					metric.errorcode = MAXTESTRUNTIME;
			}
			if(metric.errorcode)
			{
				run=0;
				break;
			}
			/*checkstall should not be called here, more than one TS might be received during one session, it is important that 
			  we check the status of the playout buffer as soon as a new frame(TS) is received
			checkstall(false);*/
		} while(still_running);
	}
	return my_curl_cleanup(prog, multi_handle, http_handle, metric.numofstreams, ITWORKED, url);

}
