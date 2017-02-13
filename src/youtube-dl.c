/*
 ============================================================================
 Name        : youtube-dl.c
 Author      : Saba
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */



#include "math.h"
#include "youtube-dl.h"
#include "helper.h"
#include "metrics.h"
#include "curlops.h"
//#include "getinfo.h"
#include "attributes.h"
#include "readmpd.h"
#include <libavformat/avformat.h>
#include <limits.h>
metrics metric;
#define PAGESIZE 500000

bool Inst = false; /*print instantaneous output through progress function if true*/
int maxtime = INT_MAX;
enum IPv ip_version = IPvSYSTEM;
int max_bitrate = INT_MAX;
int min_test_time = 0;
bool OneBitrate = false; /*If false, terminate current transfer if a stall occurs and try downloading a lower bit rate
			   If true, Continue downloading the same bit rate video even when stall occurs. */

static int check_arguments(int argc, char* argv[], char * mpdlink)
{
	for(int i=1; i<argc; i++)
	{
		if(strcmp(argv[i], "-4")==0)
			ip_version=IPv4;
		else if(strcmp(argv[i], "-6")==0)
			ip_version=IPv6;
		else if(strcmp(argv[i], "--onebitrate")==0)
			OneBitrate=true;
		else if(strcmp(argv[i], "--verbose")==0)
			Inst=true;
		else if(strcmp(argv[i], "--maxtime")==0)
		{
			maxtime = atoi(argv[++i]);
		}
		else if(strcmp(argv[i], "--maxbitrate")==0)
		{
			max_bitrate = atoi(argv[++i]) / 8;
		}
		else if(strcmp(argv[i], "--range")==0)
		{
			/*not specifying any value requests whole file. Otherwise value must be greater than LEN_CHUNK_MINIMUM seconds*/
			metric.playout_buffer_seconds = atoi(argv[++i]);
			if(metric.playout_buffer_seconds<LEN_CHUNK_MINIMUM)
				metric.playout_buffer_seconds  = LEN_CHUNK_MINIMUM;
		}
		else if(strcmp(argv[i], "--mintime")==0)
		{
			min_test_time = atoi(argv[++i]);
		}
		else if(strcmp(argv[i], "--help")==0)
		{
			printhelp(argv[0]);
			return 0;
		}

		else if(strcmp(argv[i], "--url")==0)
		{
			if(strlen(argv[++i])>MAXURLLENGTH)
			{
				printf("URL provided as argument is too long\n");
				return 0;
			}
			else
				strcpy(mpdlink, argv[i]);
		}
	}
	if(strlen(mpdlink)==0)
	{
		printf("Youtbe URL not detected\n");
		printf("To print help use the program --help switch : %s --help\n", argv[0]);

		return 0;
	}

	return 1;
}

static void signal_handler(int UNUSED(sig))
{
    metric.errorcode = SIGNALHIT;
    metric.etime= gettimelong();
    exit(0);
}

static void mainexit()
{
#ifdef DEBUG
	printf("exiting main\n");
#endif
	if(metric.errorcode==0)
		metric.errorcode = WERSCREWED;
	//printvalues();
}

static int init_libraries() {
	CURLcode ret = curl_global_init(CURL_GLOBAL_ALL);
	if(ret != CURLE_OK) {
		return -1;
	}

	av_register_all();

	return 0;
}

static int prepare_exit() {
	if(atexit(mainexit) != 0) {
		return -1;
	}

	if(signal(SIGINT, signal_handler) == SIG_ERR) {
		return -1;
	}

	if(signal(SIGTERM, signal_handler) == SIG_ERR) {
		return -1;
	}

	return 0;
}

static void init_metrics(metrics *metric) {
	memzero(metric, sizeof(*metric));
	metric->Tmin=-1;
	metric->T0 = -1;
	metric->htime = gettimelong();
	metric->Tmin0 = -1;
	metric->initialprebuftime = -1;
	metric->ft = NOTSUPPORTED;
	metric->errorcode=0;
	metric->fail_on_stall = true;
	/*if this value is set to 0, the whole file is requested. */
	metric->playout_buffer_seconds=0;

}

static void restart_metrics(metrics *metric) {
	metric->numofstalls         = 0;
	metric->totalstalltime      = 0;
	metric->initialprebuftime   = -1;
	metric->htime               = gettimelong();
	metric->TSnow               = 0;
	memset(metric->TSlist, 0, sizeof(metric->TSlist));
	metric->TS0                 = 0;
	metric->Tmin                = -1;
	metric->T0                  = -1;
	metric->Tmin0               = -1;
	metric->errorcode           = 0;
	metric->fail_on_stall       = true;
}

static size_t write_to_memory(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	static size_t num;

	char *output = userdata;
	size_t bytes = size * nmemb;

	size_t free_space = PAGESIZE - num;
	size_t written = free_space > bytes ? bytes : free_space;

	memcpy(output + num, ptr, written);
	num += written;

	return written;
}

static int download_to_memory(char url[], void *memory) {
	int ret = 0;

	CURL *curl = curl_easy_init();
	if(!curl) {
		ret = -1;
		goto out;
	}

	if(set_ip_version(curl, ip_version) < 0) {
		ret = -2;
		goto out;
	}

	CURLcode error = 0;
	error |= curl_easy_setopt(curl, CURLOPT_URL, url);
	/* if redirected, tell libcurl to follow redirection */
	error |= curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	/* timeout if no response is received in 30 seconds */
	error |= curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
	/* send all data to this function  */
	error |= curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_memory);
	/* we want the data to this memory */
	error |= curl_easy_setopt(curl, CURLOPT_WRITEDATA, memory);
	error |= curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

	if(error) {
		ret = -3;
		goto out;
	}

	/* Perform the request, error will get the return code */
	error = curl_easy_perform(curl);

	/* Check for errors  */
	if(error != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed with %d: %s\n", error,
				 curl_easy_strerror(error));

	long http_code;
	error = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	if(error || http_code != 200)
	{
		ret = -4;
		goto out;
	}

	if( curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &metric.firstconnectiontime)!= CURLE_OK)
		metric.firstconnectiontime = -1;
        else {
            double lookup_time;
        	if( curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME, &lookup_time)!= CURLE_OK) {
                	metric.firstconnectiontime=-1;
                } else {
                	metric.firstconnectiontime -= lookup_time;
                }
        }
	metric.startup = gettimelong() - metric.htime;

out:
	if(curl) curl_easy_cleanup(curl);
	return ret;
}

static int extract_media_urls(char mpdlink[]) {
	int ret = 0;

	char *pagecontent = malloc(sizeof(char [PAGESIZE]));
	if(pagecontent == NULL) {
		ret = -1;
		goto out;
	}
	memzero(pagecontent, PAGESIZE*sizeof(char));

	if(download_to_memory(mpdlink, pagecontent) < 0) {
		metric.errorcode=FIRSTRESPONSERROR;
		ret = -2;
		goto out;
	}

	ret = read_mpddata(pagecontent, mpdlink);

out:
	free(pagecontent);
	return ret;
}

int main(int argc, char* argv[])
{
	if(prepare_exit() < 0) {
		exit(EXIT_FAILURE);
	}

	if(init_libraries() < 0) {
		exit(EXIT_FAILURE);
	}

	char mpdlink[MAXURLLENGTH]="http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/4sec/BigBuckBunny_4s_simple_2014_05_09.mpd";

	init_metrics(&metric);

	if(!check_arguments(argc, argv, mpdlink))
		exit(EXIT_FAILURE);

	strncpy(metric.link, mpdlink, MAXURLLENGTH-1);
    
    /*read mpd file and get list of formats*/ 
	if ( extract_media_urls(mpdlink) < 0) {
		exit(EXIT_FAILURE);
	}
    
    for (int i = 0; i < metric.num_of_levels ; i++)
    {
        printf(" BITRATE LEVEL : %d Presenting 1st and last URL\n", metric.bitrate_level[i].bitrate);
        printf(" %s\n", metric.bitrate_level[i].segments[0]);
        printf(" %s\n", metric.bitrate_level[i].segments[metric.num_of_segments-1]);
    }
    
    /*Playing only video streams for now*/
    metric.numofstreams = 1;
    
    int ret;
    if ( ( ret = play_video())> 0) {
        printf("Return value %d\n",ret );
        exit(EXIT_FAILURE);
    }

    exit(0);

//	bool found = false;
//
//	int i=0;
//	do {
//		restart_metrics(&metric);
//
//		metric.url[0] = metric.adap_videourl[i];
//		metric.numofstreams = 1;
//
//		int j = 0;
//		do {
//			char *vformat = strstr(metric.adap_videourl[i].type, "video/") + strlen("video/");
//			char *aformat = strstr(metric.adap_audiourl[j].type, "audio/") + strlen("audio/");
//
//			if(strcmp(vformat, aformat) == 0) {
//				metric.url[1] = metric.adap_audiourl[j];
//				metric.numofstreams = 2;
//				break;
//			}
//		} while(strlen(metric.adap_audiourl[++j].url) != 0);
//
//		if(metric.url[0].bitrate + metric.url[1].bitrate > max_bitrate) {
//			continue;
//		}
//
//		if(strcasestr(metric.url[0].type, "MP4"))
//		   metric.ft = MP4;
//		else if(strcasestr(metric.url[0].type, "webm"))
//		   metric.ft = WEBM;
//		else if(strcasestr(metric.url[0].type, "flv"))
//		   metric.ft = FLV;
//		else if(strcasestr(metric.url[0].type, "3gpp"))
//		   metric.ft = TGPP;
//		if(metric.numofstreams > 1) {
//		   if(metric.ft == MP4) {
//			   metric.ft = MP4_A;
//		   } else if(metric.ft == WEBM) {
//			   metric.ft = WEBM_A;
//		   }
//		}
//
//		if(strlen(metric.adap_videourl[i + 1].url) == 0 &&
//		   strlen(metric.no_adap_url[0].url) == 0 || OneBitrate) {
//			metric.fail_on_stall = false;
//		}
//
//		downloadfiles(metric.url);
//
//		if(metric.errorcode == ITWORKED &&
//		   metric.downloadtime[STREAM_VIDEO] < min_test_time) {
//			metric.errorcode = TOO_FAST;
//		}
//
//		if(metric.fail_on_stall &&
//		   (metric.errorcode == ERROR_STALL || metric.errorcode == TOO_FAST)) {
//			printvalues();
//			continue;
//		}
//
//		found = true;
//		break;
//	} while(strlen(metric.adap_videourl[++i].url) != 0);
//
//	if(!found) {
//		memset(&metric.url[STREAM_AUDIO], 0, sizeof(metric.url[STREAM_AUDIO]));
//		memset(&metric.cdnip[STREAM_AUDIO], 0, sizeof(metric.cdnip[STREAM_AUDIO]));
//		metric.downloadtime[STREAM_AUDIO] = 0;
//		metric.totalbytes[STREAM_AUDIO] = 0;
//		metric.TSlist[STREAM_AUDIO] = 0;
//		metric.downloadrate[STREAM_AUDIO] = 0;
//		metric.connectiontime[STREAM_AUDIO] = 0;
//
//		metric.numofstreams = 1;
//
//		i=0;
//		do {
//			restart_metrics(&metric);
//
//			metric.url[0] = metric.no_adap_url[i];
//
//			if(metric.url[0].bitrate > max_bitrate) {
//				continue;
//			}
//
//			if(strcasestr(metric.url[0].type, "MP4"))
//			   metric.ft = MP4;
//			else if(strcasestr(metric.url[0].type, "webm"))
//			   metric.ft = WEBM;
//			else if(strcasestr(metric.url[0].type, "flv"))
//			   metric.ft = FLV;
//			else if(strcasestr(metric.url[0].type, "3gpp"))
//			   metric.ft = TGPP;
//
//			if(strlen(metric.no_adap_url[i + 1].url) == 0 || OneBitrate) {
//				metric.fail_on_stall = false;
//			}
//
//			downloadfiles(metric.url);
//
//			if(metric.errorcode == ITWORKED &&
//			   metric.downloadtime[STREAM_VIDEO] < min_test_time) {
//				metric.errorcode = TOO_FAST;
//			}
//
//			if(metric.fail_on_stall &&
//			   (metric.errorcode == ERROR_STALL || metric.errorcode == TOO_FAST)) {
//				printvalues();
//				continue;
//			}
//
//			found = true;
//			break;
//		} while(strlen(metric.no_adap_url[++i].url) != 0);
//	}
//
//	if(metric.errorcode == 403) {
//		return 148;
//	} else if(metric.errorcode == TOO_FAST) {
//		return 149;
//	} else {
//		return 0;
//	}
}
