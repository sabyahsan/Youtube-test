/*
 * youtube-dl.h
 *
 *  Created on: Feb 28, 2013
 *      Author: sahsan
 */

#ifndef YOUTUBE_DL_H_
#define YOUTUBE_DL_H_
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <pthread.h>
#include <curl/curl.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>


#define ITWORKED 600
#define WERSCREWED 666
#define BADFORMATS 601 //we didn't find the right format for download.
#define CURLERROR 602
#define MAXDOWNLOADLENGTH 603 /*three minutes of video has been downloaded*/
#define MAXTESTRUNTIME 604 /*test has been downloading video for the maximum time */
#define SIGNALHIT 605 /* program received ctrl+c*/
#define CODETROUBLE 606 /*HTTP code couldn't be retrieved*/
#define ERROR_STALL 607 /* There was a stall */
#define TOO_FAST 608 /* The test finsihed too fast */

#define NUMOFSTREAMS 2
#define STREAM_VIDEO 0
#define STREAM_AUDIO 1



#define NUMOFCODES 9
#define MIN_PREBUFFER 2000 /* in millisecond*/
#define MIN_STALLBUFFER 1000
#define MAXURLLENGTH 512

/*the number of filetype enum indicates priority, the lower the value, the higher the priority*/
typedef enum {MP4=3, WEBM=4, FLV=5, TGPP=6, MP4_A=1, WEBM_A=2, NOTSUPPORTED=7} filetype;

#define URLTYPELEN 100
#define URLSIZELEN 12
#define URLSIGLEN 100
#define CDNURLLEN 1500
#define URLLISTSIZE 24

typedef struct
{
	int itag;
	char url[CDNURLLEN];
	char type[URLTYPELEN];
	int bitrate;
} videourl;

#define CHARPARSHORT 12
#define CHARPARLENGTH 24
#define CHARSTRLENGTH 256
#define FORMATLISTLEN 100


typedef struct
{
	videourl url[NUMOFSTREAMS];
	videourl adap_videourl[URLLISTSIZE];
	videourl adap_audiourl[URLLISTSIZE];
	videourl no_adap_url[URLLISTSIZE];
	filetype ft;
	int numofstreams;
	char link[MAXURLLENGTH];
	time_t htime; /*unix timestamp when test began*/
	long long stime; /*unix timestamp in microseconds*/
	long long etime;/*unix timestamp in microseconds*/
	//char url[CDNURLLEN];
	int numofstalls;
	char cdnip[NUMOFSTREAMS][CHARSTRLENGTH];
	double totalstalltime; //in microseconds
	double initialprebuftime; //in microseconds
	double downloadtime[NUMOFSTREAMS]; //time take for video to download
	double totalbytes[NUMOFSTREAMS];
	uint64_t TSnow; //TS now (in milliseconds)
	uint64_t TSlist[NUMOFSTREAMS]; //TS now (in milliseconds)
	uint64_t TS0; //TS when prebuffering started (in milliseconds). Would be 0 at start of movie, but would be start of stall time otherwise when stall occurs.
	long long Tmin; // microseconds when prebuffering done or in other words playout began.
	long long Tmin0; //microseconds when prebuffering started, Tmin0-Tmin should give you the time it took to prebuffer.
	long long T0; /*Unix timestamp when first packet arrived in microseconds*/
	int dur_spec; /*duration in video specs*/
	double downloadrate[NUMOFSTREAMS];
	int errorcode;
	double connectiontime[NUMOFSTREAMS];
	bool fail_on_stall;
} metrics;

enum IPv {IPvSYSTEM, IPv4, IPv6};

#endif /* YOUTUBE_DL_H_ */
