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
	printf("\t<url> => URL of the YouTube video\n");
	printf("\t--nodownload => Only get the video information, actual video is not downloaded\n");
	printf("\n");
	printf("\t--verbose => Print instantaneous metric values when downloading video\n\t\tHas no effect if --nodownload is used\n");
	printf("\n");
	printf("\t--range <x>=> The length of the playout buffer in seconds. If no value is given whole file is downloaded\n");
	printf("\t\t otherwise only x worth of video is buffered at any time. Value of x must be greater than 5, smaller values\n\t\t default to 5 automatically\n"); 
	printf("\n");
	printf("\t--onebitrate => when used the client does not switch to lower bit rate when a stall occurs\n");
	printf("\n");
	printf("RUN WITHOUT ARGUMENTS FOR TESTING.\n");
	exit(0); 

}

