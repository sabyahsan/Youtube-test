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


int set_ip_version(CURL *curl, enum IPv ip_version)
{
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

int curle_success(CURLcode eret)
{
    if(eret != CURLE_OK)
    {
        printf("curl_easy_setopt() failed with %d: %s\n", eret, curl_easy_strerror(eret));fflush(stdout);
        return 0;
    }
    else
        return 1;
}

int curlm_success(CURLMcode mret)
{
    if(mret != CURLM_OK)
    {
        printf("curl_multi_setopt() failed with %d: %s\n", mret, curl_multi_strerror(mret));fflush(stdout);
        return 0;
    }
    else
    {
        return 1;
    }
}


static size_t receive_stream ( void * ptr, size_t size, size_t nmemb, void * userdata)
{
    struct myprogress * p = (struct myprogress *)userdata;
    size_t len = size * nmemb;
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
    }
    else if(p->stream == STREAM_AUDIO)
    {
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

int initialize_curl_for_next_segment( CURL ** http_handle_ref, int streamid, struct myprogress * prog, CURLM *multi_handle, int curr_segment, int curr_level)
{
    CURL * http_handle;
    if(*http_handle_ref != NULL)
    {
        http_handle = *http_handle_ref;
        curl_multi_remove_handle(multi_handle, http_handle);
        curl_easy_cleanup (http_handle);
    }
    else
    {
        prog->stream = streamid;
        prog->init = false;
    }
    prog->lastdlbytes = 0;
    prog->lastruntime = 0;
    
    if((http_handle = curl_easy_init())==NULL)
    {
        printf("Curl initialization failed \n");fflush(stdout);
        return -1;
    }
    
    *http_handle_ref = http_handle;
    
    
    /* set options */

    if(!curle_success(curl_easy_setopt(http_handle, CURLOPT_URL, metric.bitrate_level[curr_level].segments[curr_segment])))
    {
        curl_easy_cleanup (http_handle);
        return -1;
    }
    if(!curle_success(curl_easy_setopt(http_handle, CURLOPT_WRITEFUNCTION, receive_stream)))
    {
        curl_easy_cleanup (http_handle);
        return -1;
    }
    if(!curle_success(curl_easy_setopt(http_handle, CURLOPT_WRITEDATA, (void *)prog)))
    {
        curl_easy_cleanup (http_handle);
        return -1;
    }
    
    if(!curlm_success(curl_multi_add_handle(multi_handle, http_handle)))
    {
        curl_easy_cleanup (http_handle);
        return -1;
    }
    prog->curl = http_handle;

    return 1;
}


int play_video()
{
    CURL * http_handle          = NULL;
    CURLM * multi_handle        = NULL;
    int still_running;
    int curr_segment            = 0;
    int curr_bitrate_level      = 0;
    struct myprogress prog;

    
    memzero(&prog, sizeof(struct myprogress));
    
    metric.stime = gettimelong();
    printf("Starting download 1\n"); fflush(stdout);
    
    if((multi_handle = curl_multi_init())==NULL)
    {
        printf("curl_multi_init() failed and returned NULL\n");
        return -1;
    }
    while (curr_segment != metric.num_of_segments)
    {
        printf("Initiating download of segment %d :\n", curr_segment); fflush(stdout);

        if(!initialize_curl_for_next_segment(&http_handle, 0, &prog, multi_handle, curr_segment, curr_bitrate_level))
        {
            curl_multi_cleanup(multi_handle);
            return CURLERROR;
        }
        if(!curlm_success(curl_multi_perform(multi_handle, &still_running)))
            return CURLERROR;
        
        do {
            struct timeval timeout;
            int rc; /* select() return code */
            CURLMcode mc; /* curl_multi_fdset() return code */
            
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
            
            curl_multi_timeout(multi_handle, &curl_timeo);
            if(curl_timeo >= 0) {
                timeout.tv_sec = curl_timeo / 1000;
                if(timeout.tv_sec > 1)
                timeout.tv_sec = 1;
                else
                timeout.tv_usec = (curl_timeo % 1000) * 1000;
            }
            
            mc = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);
            
            if(mc != CURLM_OK) {
                printf("curl_multi_fdset() failed, code %d.\n", mc);
                break;
            }
            
            if(maxfd == -1) {
    #ifdef _WIN32
                Sleep(100);
                rc = 0;
    #else
                /* Portable sleep for platforms other than Windows. */
                struct timeval wait = { 0, 100 * 1000 }; /* 100ms */
                rc = select(0, NULL, NULL, NULL, &wait);
    #endif
            }
            else {
                /* Note that on some platforms 'timeout' may be modified by select().
                 If you need access to the original value save a copy beforehand. */
                rc = select (maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
            }
            
            switch(rc) {
                    case -1:
                    /* select error */
                    break;
                    case 0:
                default:
                    /* timeout or readable/writable sockets */
                    curl_multi_perform (multi_handle, &still_running);
                    break;
            }
        } while(still_running);
        printf("Segment fully downloaded\n"); fflush(stdout);
        curr_segment ++;
    }
    
    curl_multi_remove_handle(multi_handle, http_handle);

    curl_easy_cleanup (http_handle);

    curl_multi_cleanup(multi_handle);
    
    
    return 0;
    
}

static size_t save_init_segments (void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize     = size * nmemb;
    dash_init_segment * init      = (dash_init_segment *) userp;
    
    if (init->size + realsize >= MAX_DASH_INIT_SEGMENT_SIZE)
    {
        printf("Not enough memory to store the init segments, consider checking MAX_DASH_INIT_SEGMENT_SIZE\n");
        return 0;
    }
    memcpy(init->data+init->size , contents, realsize);
    init->size += realsize;
    return realsize;
}

int download_init_segments()
{
    CURL **http_handle  = malloc(metric.num_of_levels * sizeof(CURL *));
    memzero(http_handle,metric.num_of_levels * sizeof(CURL *));
    CURLM *multi_handle;
    
    int still_running;
    
    int run = metric.numofstreams;
    
    metric.stime = gettimelong();
    printf("Starting download 1\n"); fflush(stdout);
    
    for (int i = 0; i < metric.num_of_levels; i++)
    //for (int i = 0; i < 2; i++)
    {
        printf("Initializing curl handle # %d\n", i); fflush(stdout);
        
        if((http_handle[i] = curl_easy_init())==NULL)
        {
            printf("Curl initialization failed \n");fflush(stdout);
            return CURLERROR;
        }
        if(!curle_success(curl_easy_setopt(http_handle[i], CURLOPT_URL, metric.bitrate_level[i].segments[0])))
        return CURLERROR;
        
        if(!curle_success(curl_easy_setopt(http_handle[i], CURLOPT_WRITEFUNCTION, save_init_segments)))
        return CURLERROR;
        
        if(!curle_success(curl_easy_setopt(http_handle[i], CURLOPT_WRITEDATA, (void *)&(metric.bitrate_level[i].init))))
        return CURLERROR;
    }
    
    if((multi_handle = curl_multi_init())==NULL)
    {
        printf("curl_multi_init() failed and returned NULL\n");
        return CURLERROR;
    }
    
    for (int i = 0; i < metric.num_of_levels; i++)
    {
        if(!curle_success(curl_multi_add_handle(multi_handle, http_handle[i])))
        return CURLERROR;
    }
    
    printf("Starting download\n"); fflush(stdout);
    
    if(!curlm_success(curl_multi_perform(multi_handle, &still_running)))
    return CURLERROR;
    
    do {
        struct timeval timeout;
        int rc; /* select() return code */
        CURLMcode mc; /* curl_multi_fdset() return code */
        
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
        
        curl_multi_timeout(multi_handle, &curl_timeo);
        if(curl_timeo >= 0) {
            timeout.tv_sec = curl_timeo / 1000;
            if(timeout.tv_sec > 1)
            timeout.tv_sec = 1;
            else
            timeout.tv_usec = (curl_timeo % 1000) * 1000;
        }
        
        mc = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);
        
        if(mc != CURLM_OK) {
            printf("curl_multi_fdset() failed, code %d.\n", mc);
            break;
        }
        
        if(maxfd == -1) {
#ifdef _WIN32
            Sleep(100);
            rc = 0;
#else
            /* Portable sleep for platforms other than Windows. */
            struct timeval wait = { 0, 100 * 1000 }; /* 100ms */
            rc = select(0, NULL, NULL, NULL, &wait);
#endif
        }
        else {
            /* Note that on some platforms 'timeout' may be modified by select().
             If you need access to the original value save a copy beforehand. */
            rc = select (maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
        }
        
        switch(rc) {
                case -1:
                /* select error */
                break;
                case 0:
            default:
                /* timeout or readable/writable sockets */
                curl_multi_perform (multi_handle, &still_running);
                break;
        }
    } while(still_running);
    
    curl_multi_cleanup(multi_handle);
    
    for (int i = 0; i < metric.num_of_levels; i++)
    {
        curl_easy_cleanup (http_handle[i]);
    }
    
    free (http_handle);
    
    
    return 0;
    
}
