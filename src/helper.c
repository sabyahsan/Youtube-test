/*
 *
 *      Year   : 2013-2017
 *      Author : Saba Ahsan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
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
	printf("%s <arguments>\n", name);
	printf("\n");
	printf("Arguments\n");
	printf("\t<url> => URL of the YouTube video\n");
	printf("\n");
	printf("\t--verbose => Print instantaneous metric values when downloading video\n\t\tHas no effect if --nodownload is used\n");
	printf("\n");
	printf("\t--range <s> => The length of the playout buffer in seconds. Default value is 5; value of x must be greater than 5 \n");
	printf("\n");
	printf("\t--onebitrate => when used the client does not switch to lower bit rate when a stall occurs. Disabled by default.\n");
	printf("\n");
    printf("\t--maxbitrate <bps> => if specified, the client will not attempt to download qualities with a higher bit rate, default MAXINT\n");
	printf("\n");
    printf("\t--mintime <x> => if a test finishes before this time, it is considered a failure, default value is 0\n");
	printf("\n");
    printf("\t--maxtime <x> => maximum time for which the test should run, default value is MAXINT\n");
	printf("\n");
	printf("\t-4 => Use IPv4 only\n");
	printf("\n");
	printf("\t-6 => Use IPv6 only\n");
	printf("\n");
	printf("RUN WITHOUT ARGUMENTS FOR TESTING.\n");

}

