#include "getinfo.h"
#include "helper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <regex.h>
#include "youtube-dl.h"
#include "adaptive.h"
#include "mm_parser.h"
#include "curlops.h"
extern metrics metric;
#define ITAGLEN 7
#define MANIFESTLEN 1024
#define MSGBUFLEN 100
/*Itag information :
 *      '5': '240x400',
        '6': '???',
        '13': '???',
        '17': '144x176', mp4
        '18': '360x640', mp4
        '22': '720x1280', mp4
        '34': '360x640', flv   [DEFAULT IN YOUTUBE]
        '35': '480x854', flv
        '37': '1080x1920', mp4
        '38': '3072x4096', video ??
        '43': '360x640', webm
        '44': '480x854', webm
        '45': '720x1280', webm
        '46': '1080x1920', webm
 */

static int extract_duration(char * data , int * duration)
{
    regex_t regex;
    int reti;
    char msgbuf[MSGBUFLEN];
    regmatch_t m;
    int start=0, finish=0;
    char tmp[CHARSTRLENGTH];
    memzero(tmp, sizeof(tmp));

    /* Compile regular expression */
    reti = regcomp(&regex, "itemprop[ \t]*=[ \t]*\"[ \t]*duration.*$", REG_NEWLINE|REG_EXTENDED);
    if( reti )
    {
    	fprintf(stderr, "Could not compile regex\n");
    	return 0;
    }


    /* Execute regular expression */
    reti = regexec(&regex, data, 1, &m, 0);
    if( !reti )
    {
    	start = m.rm_so ;
    	finish = m.rm_eo;
    }
    else if( reti == REG_NOMATCH )
    {
    //	puts("No match");
        regfree(&regex);
    	return 0;
    }

    else{
    	regerror(reti, &regex, msgbuf, sizeof(msgbuf));
    	fprintf(stderr, "Regex match failed: %s\n", msgbuf);
        regfree(&regex);
    	return 0;
    }
//    data[finish]='\0';
//    printf ("%s (bytes %d:%d - %d)\n", data + start, start, finish, finish-start);
    regfree(&regex);
    /*copy one less than full lenght so that last char is still nULL*/
    strncpy(tmp, data+start, ((finish-start)<CHARSTRLENGTH ? (finish-start) : (CHARSTRLENGTH-1)));
//    printf ("TMP %s (bytes %d:%d)\n", tmp, start, finish);

    char * tmp2, *tmp3;
    tmp2 = strstr(tmp, "content");
    if(tmp2==NULL)
    {
    	return 0;
    }
    tmp3 = strstr(tmp2, "\"");
    if(tmp3==NULL)
    {
    	return 0;
    }
    tmp2 = strstr(tmp3+1, "\"");
    if(tmp2==NULL)
    {
    	return 0;
    }
//    printf ("TMP2 %s\nTMP3 %s\n", tmp2, tmp3);
    int min, sec;
    *tmp2='\0';
    tmp3=tmp3+3;
    tmp2 = strstr(tmp3, "M");
    if(tmp2==NULL)
    	min=0;
    else
    {
    	*tmp2='\0';
    	min=atoi(tmp3);
 //   	printf("%s %d \n", tmp3 ,min);
    	tmp3=tmp2+1;
    }
    tmp2 = strstr(tmp3+1, "S");
    if(tmp2==NULL)
    	sec=0;
    else
    {
    	*tmp2='\0';
    	sec=atoi(tmp3);
  //     	printf("%s %d \n", tmp3, sec);
         	tmp3=tmp2+1;
    }

    *duration = (min*60)+sec;
    return 1;
}

static bool replace_html_codes(char * data)
{
	char codes[NUMOFCODES][4] = {"%3A","%2F","%3F","%3D","%26","%25","%22","%2C","%3B"};
	char symbols[NUMOFCODES][2] = { ":", "/", "?", "=", "&", "%", "\"", ",", ";"};
	char * result;
	char * orig = data;
	if(data==NULL || strlen(data)<10){
		if(data==NULL)
			printf("Data is NULL\n");
		else
			printf("strlen is too short for %s\n", data);
		return false;

	}
	int i;
	for (i=0 ; i<NUMOFCODES ; i++)
	{
		result = str_replace(orig, codes[i], symbols[i]);
		if(result ==NULL)
			continue;
		//if orig is not pointing to data, then free it for the next iteration.
		if(orig!=data)
			free(orig);
		orig = result;
	}
#ifdef DEBUG
	printf("URL encodings removed \n");
#endif

	strcpy(data,orig);
	free(orig);
	return true;
//	printf("New string \n %s", orig);
}

static int extract_bitrate(char *data , int *bitrate) {
    regex_t regex;
    int reti;
    char msgbuf[MSGBUFLEN];
    regmatch_t m;
    int start;
    char tmp[1024];
    memzero(tmp, sizeof(tmp));

    /* Compile regular expression */
    reti = regcomp(&regex, "bitrate=[0-9]+", REG_EXTENDED);
    if( reti )
    {
    	fprintf(stderr, "Could not compile regex\n");
    	return 0;
    }


    /* Execute regular expression */
    reti = regexec(&regex, data, 1, &m, 0);
    if( !reti )
    {
    	start = m.rm_so ;
    }
    else if( reti == REG_NOMATCH )
    {
    	perror("No match type (bitrate)");
        regfree(&regex);
    	return 0;
    }

    else{
    	regerror(reti, &regex, msgbuf, sizeof(msgbuf));
    	fprintf(stderr, "Regex match failed: %s\n", msgbuf);
        regfree(&regex);
    	return 0;
    }
  //  data[finish]='\0';
   // printf ("%s (bytes %d:%d - %d)\n", data + start, start, finish, finish-start);
    regfree(&regex);

    *bitrate = atoi(data + start + strlen("bitrate="));
    *bitrate /= 8;

    return 1;
}

static int extract_formattype(char * data , char * type)
{
    regex_t regex;
    int reti;
    char msgbuf[MSGBUFLEN];
    regmatch_t m;
    int start, finish;
    char tmp[1024];
    memzero(tmp, sizeof(tmp));

    /* Compile regular expression */
    reti = regcomp(&regex, "((^t)|([^a-zA-Z_-]t))ype=[^\\+. ;]+", REG_NEWLINE|REG_EXTENDED);
    if( reti )
    {
    	fprintf(stderr, "Could not compile regex\n");
    	return 0;
    }


    /* Execute regular expression */
    reti = regexec(&regex, data, 1, &m, 0);
    if( !reti )
    {
    	start = m.rm_so ;
    	finish = m.rm_eo;
    }
    else if( reti == REG_NOMATCH )
    {
    	perror("No match type (type)");
        regfree(&regex);
    	return 0;
    }

    else{
    	regerror(reti, &regex, msgbuf, sizeof(msgbuf));
    	fprintf(stderr, "Regex match failed: %s\n", msgbuf);
        regfree(&regex);
    	return 0;
    }
  //  data[finish]='\0';
   // printf ("%s (bytes %d:%d - %d)\n", data + start, start, finish, finish-start);
    regfree(&regex);
    strncpy(tmp, data+start, ((size_t)(finish-start)<sizeof(tmp) ? (size_t)(finish-start) : (sizeof(tmp)-1)));
 //   printf ("TMP %s (bytes %d:%d)\n\n", tmp, start, finish);
    char * tmp2 = strstr(tmp, "=");
    if(tmp2==NULL)
    {
       	return 0;
    }
    else if(strlen(tmp2)<100)
	{
		strncpy(type, tmp2+1, URLTYPELEN-1);
		type[URLTYPELEN-1]='\0';
		replace_html_codes(type);
//		if(strstr(type, "mp4") )
    //	printf ("TMP %d: %s \n", strlen(tmp2), tmp2);
	}
    return 1;
}

static int extract_urlstring(char * data, int * start, int * finish)
{
    regex_t regex;
    int reti;
    char msgbuf[MSGBUFLEN];
    regmatch_t m;

    /* Compile regular expression */
    reti = regcomp(&regex, "url_encoded_fmt_stream_map[ \t]*\":[ \t]*\"[^\"]*", REG_NEWLINE|REG_EXTENDED);
    if( reti )
    {
    	fprintf(stderr, "Could not compile regex\n");
    	return 0;
    }



    /* Execute regular expression */
    reti = regexec(&regex, data, 1, &m, 0);
//	cout<<"Ran regex "<<endl;fflush(stdout);

    if( !reti )
    {
    	*start = m.rm_so ;
    	*finish = m.rm_eo;
    }
    else if( reti == REG_NOMATCH )
    {
    	//puts("No match stream map");
        regfree(&regex);
        return 0;
    }

    else{
    	regerror(reti, &regex, msgbuf, sizeof(msgbuf));
    	fprintf(stderr, "Regex match failed: %s\n", msgbuf);
        regfree(&regex);
        return 0;
    }
#ifdef DEBUG
    printf ("'%.*s' (bytes %d:%d)\n", (*finish - *start),
                        data + *start, *start, *finish);
#endif

//	if(log)
//	{
//		writelog(data+*start, strlen(data+*start));
//	}
//	data[*finish]='\0';
//#ifdef DEBUG
//    printf ("'%s' (bytes %d:%d)\n", data + *start, *start, *finish);
//#endif
    /* Free compiled regular expression */
    regfree(&regex);
    return 1;
}

/*Extract the contents from split, and copies to the structure url
 * returns true if contents are valid and copied, else false
 */
static bool copy_to_videourl(char * split, videourl * url)
{

	char *tmp=NULL, *tmp2=NULL;
	tmp = strstr(split, "url=");
	if(tmp!=NULL)
	{
		if(strlen(tmp)>4)
			tmp+=4;
		else
			return false;
		tmp2=strstr(tmp, "\\u0026");
		if(tmp2!=NULL)
		{
			strncpy(url->url, tmp, (tmp2-tmp));
			url->url[tmp2-tmp]='\0';
		}
		else
		{
			strncpy(url->url, tmp,1500);
		}

#ifdef DEBUG

		printf("The %d url is %s \n",url->itag, url->url);
#endif
	}

#ifdef DEBUG
	else {
		printf("no url found:  %d\n", url->itag );
		return false;
	}
#endif

	return true;
}

/* If no valid URL is found, -1 is returned
 * Otherwise the number of format urls found is returned and the urls are all saved to url_list;
 */
static int split_urls(char * data, videourl url_list[], bool adaptive)
{
	char * split;
	int index=0;
	char *tmp=NULL;
	char itagstr[ITAGLEN];
	memzero(itagstr, sizeof(itagstr));

	/* replace HTML code for = signs but not the rest.
	 * Specifically I don't want to make the replacements for comma ,
	 * so that the splitting of urls is easy.
	 */

	split = strtok(data, ",");
	if(split==NULL)
		split=data;

	do
	{

		/*extract the itag and save to url list*/
//		printf("Looking for itag 1\n");
		tmp = strstr(split, "itag%3D");
		if(tmp!=NULL)
		{
	//		printf("Found itag 1\n");

			tmp+=7;
			strncpy(itagstr, tmp, (ITAGLEN>strlen(tmp) ? strlen(tmp) : ITAGLEN));
			itagstr[ITAGLEN-1]='\0';
			url_list[index].itag = strtol(itagstr, (char **)NULL, 10);
		}
		else
		{
	//		printf("Looking for itag 2\n");

			tmp = strstr(split, "itag=");
			if(tmp!=NULL)
			{
	//			printf("Found itag 2\n");

				tmp+=4;
				strncpy(itagstr, tmp, ITAGLEN);
				itagstr[ITAGLEN-1]='\0';
				url_list[index].itag = strtol(itagstr, (char **)NULL, 10);;
			}
			else
			{

#ifdef DEBUG
				printf("no itag found for index %d\n", index );
#endif
				continue;
			}
		}

		extract_formattype(split , url_list[index].type);

		if(adaptive) 
			extract_bitrate(split, &url_list[index].bitrate);

		if(!copy_to_videourl(split,url_list+index))
		{
#ifdef DEBUG
			printf("no url found for index %d\n", index );
#endif
			continue;
		}
		replace_html_codes(url_list[index].url);

		++index;
//		printf("Moving on\n");

	}while(split=strtok(NULL, ","), split!=NULL);

//	printf("formatlist is %s\n", formatlist);
//	for (int i =0; i< index; i++)
//		cout<<"Itag "<<i<<" "<<url_list[i].itag<<" "<<url_list[i].url<<endl;

	return index;
}

static int getbitrate(const char url[]) {
	int ret = 0;

	long size = getfilesize(url);
	if(size <= 0) {
		ret = -1;
		goto out;
	}

	long bytes_sec = size / metric.dur_spec;

	ret = bytes_sec;
out:

	return ret;
}

/*extract the list of urls and saves them to a videourl list,
 * returns pointer to the videourl array or NULL if it fails.
 * index contains the number of urls in the list. Cannot extract
 * more than URLLISTSIZE urls.
 */
static videourl * extract_url_list(char * data, int * index, bool adaptive)
{
	videourl * url_list= malloc(sizeof(videourl [URLLISTSIZE]));
	memzero(url_list, URLLISTSIZE*sizeof(videourl));
	char * datatmp;
	int start, finish;
	bool available;
	if(adaptive)
		available = extract_urlstring_adaptive(data, &start, &finish);
	else
		available = extract_urlstring(data, &start, &finish);
//	cout<<"Return from adaptive is 1 "<<endl;fflush(stdout);
	if(available)
	{
		datatmp=malloc(sizeof(char[finish-start+1]));
		strncpy(datatmp, data+start, finish-start);
		datatmp[finish-start]='\0';
		*index = split_urls(datatmp, url_list, adaptive);
		if(*index<0)
		{
			free(url_list);
			url_list= NULL;
		}
		free(datatmp);
	}

	return url_list;
}

static int compar_bitrate(const void *left, const void *right) {
	videourl *myleft = (videourl *)left;
	videourl *myright = (videourl *)right;

	return myright->bitrate - myleft->bitrate;
}

static void findformat(char * data)
{
	int index_adaptive=0, index_nonadaptive=0;

	/*get the url lists populated*/
	videourl * url_list = extract_url_list(data, &index_nonadaptive, false);
	videourl * url_list_adaptive =extract_url_list(data, &index_adaptive, true);

	for(int i =0 ; i<index_nonadaptive; ++i) {
		url_list[i].bitrate = getbitrate(url_list[i].url);
	}

	memcpy(metric.no_adap_url, url_list, index_nonadaptive * sizeof(*url_list));
	memset(&metric.no_adap_url[index_nonadaptive], 0, sizeof(*metric.no_adap_url));

	int a_index = 0;
	int v_index = 0;
	for(int i = 0; i < index_adaptive; ++i) {
		if(strstr(url_list_adaptive[i].type, "audio")) {
			metric.adap_audiourl[a_index++] = url_list_adaptive[i];
		} if(strstr(url_list_adaptive[i].type, "video")) {
			metric.adap_videourl[v_index++] = url_list_adaptive[i];
		}
	}
	memset(&metric.adap_audiourl[a_index], 0, sizeof(*metric.adap_audiourl));
	memset(&metric.adap_videourl[v_index], 0, sizeof(*metric.adap_videourl));

	if(url_list!=NULL)
		free(url_list);
	if(url_list_adaptive!=NULL)
		free(url_list_adaptive);

	qsort(metric.no_adap_url, index_nonadaptive, sizeof(*metric.no_adap_url), compar_bitrate);
	qsort(metric.adap_audiourl, a_index, sizeof(*metric.adap_audiourl), compar_bitrate);
	qsort(metric.adap_videourl, v_index, sizeof(*metric.adap_videourl), compar_bitrate);
}

/* return value of 1 indicates non-adaptive format
 * 2 indicates adaptive format with 2 streams
 * -1 indicates failure to find a good url
 * if bool adaptive is false, adaptive formats are not considered.
 */

void find_urls(char * data)
{
//	char manifest[MANIFESTLEN];
//	extract_manifestlink(data , manifest);
	//cout<<"strlength data "<<strlen(data)<<endl;
//	download_manifest(manifest);
	if(!extract_duration(data, &metric.dur_spec))
		metric.dur_spec=-1;
	/*Get the stream URLs*/

	findformat(data);

	return;

}
