/*
 * helper.cpp
 *
 *  Created on: Feb 28, 2013
 *      Author: sahsan
 */


#include "helper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#include "youtube-dl.h"

/*http://stackoverflow.com/questions/779875/what-is-the-function-to-replace-string-in-c
 * Function returns NULL when there is no replacement to be made.
 */

char *str_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep
    int len_with; // length of with
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements
//	cout<<"woah boy replacing "<<rep<<" with "<<with;

    if (!orig)
    {
//    	cout<<" not found "<<endl;
        return NULL;
    }
    if (!rep || !(len_rep = strlen(rep)))
    {
//    	cout<<" not found "<<endl;
        return NULL;
    }
    if (!(ins = strstr(orig, rep)))
    {
//    	cout<<" not found "<<endl;
        return NULL;
    }
    if (!with)
        with[0] = '\0';
    len_with = strlen(with);
    tmp = strstr(ins, rep);
    for (count = 0; tmp ; ++count) {
        ins = tmp + len_rep;
        tmp = strstr(ins, rep);
    }

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    tmp = result = (char *)malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
    {
//    	cout<<" not found "<<endl;
        return NULL;
    }
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
//	cout<<" done "<<endl;

    return result;
}

void printhelp(char * name)
{
	printf("You have used the --help switch so here is the help:\n\n");

	printf("The program uses the configuration file configyt and default values to run, alternatively arguments can be provided to override both configyt and defaults:\n\n");

	printf("Usage of the program:\n");
	printf("\n");
	printf("%s <optional-parameters>\n", name);
	printf("\n");
	printf("Optional parameters\n");
	printf("\t--nodownload => Only get the video information, actual video is not downloaded\n");
	printf("\n");
	printf("\t--verbose => Print instantaneous metric values when downloading video\n\t\tHas no effect if --nodownload is used\n");
	printf("\n");
	printf("\t--flv => Download the default FLV version even if other formats are available\n");

	printf("\nThe output contains some information about the test and also various kinds of records that can be identified by the first column of the record"
			"which can be 1, 2, 3 etc.. The fields are given below\n\n");
	printf("Type: 1 or 0 ; printed before downloading the video. If the video URLs were extracted successfully it is 1 else 0\n");
	printf("1\turl\tcategory\tduration\tviews\tlikes\tdislikes\tformatlist\tadaptiveavailable\ttitle\tuploaddate\n\n");

	printf("Type: 2 prints the final metrics for the downloaded video\n");
	printf("2\tDownloadTime\tNumberOfStalls\tAverageStallDuration\tTotalStallTime\tAveThroughput\tVideoRate\tAudioRate\tDuration\tInitialPrebufferingtime(us)\t"
			"TotalMediaRate\tTotalBytes\tMaxMediaRate\n\n");

	printf("Type: 3 prints some instantaneous metrics every second ONLY if --verbose is used \n");
	printf("3;Timenow;LastTSreceived;PercentageOfVideoDownloaded;TotalbytesDownloaded;TotalBytestoDownload;InstantenousThrouput;"
			"AverageThroughput;AverageMediaRate;NumberofStalls;AverageStallDuration;TotalStallTime\n\n");

	printf("Type: 6 prints the instantaneous mediarate \n");
	printf("6\tMediarate(kbps)\tTimestampNow(microseconds)\tCumulativeMediarate(kbps)\n\n");

	printf("Type: 12 prints the stall information for each stall (Times in milliseconds) \n");
	printf("12\tStall-starttime(videoplayout)\tStallDuration(msec)\n\n");

	printf("Type: 4 prints the TCP stats of video download \n");
	printf("4\ttcp_info.tcpi_last_data_sent\t\
		tcp_info.tcpi_last_data_recv\t\
		tcp_info.tcpi_snd_cwnd\t\
		tcp_info.tcpi_snd_ssthresh\t\
		tcp_info.tcpi_rcv_ssthresh\t\
		tcp_info.tcpi_rtt\t\
		tcp_info.tcpi_rttvar\t\
		tcp_info.tcpi_unacked\t\
		tcp_info.tcpi_sacked\t\
		tcp_info.tcpi_lost\t\
		tcp_info.tcpi_retrans\t\
		tcp_info.tcpi_fackets\n"
	   );


	printf("When the --getsizes option is used the sizes are printed out one line for each format as follows  \n");
	printf("filesize\titag\tfilesize(bytes)\tcdnip\n\n");

	printf("DO NOT ATTEMPT TO CHANGE configyt IF YOU DON't KNOW WHAT YOU ARE DOING.\n");
	printf("RUN WITHOUT ARGUMENTS FOR TESTING.\n");

}

