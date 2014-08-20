/*
 * adaptive.cpp
 *
 *  Created on: Oct 24, 2013
 *      Author: sahsan
 */

#include "adaptive.h"
#include "youtube-dl.h"

#include <regex.h>

int extract_urlstring_adaptive(char * data, int * start, int * finish)
{
    regex_t regex;
    int reti;
    char msgbuf[100];
    regmatch_t m;

    /* Compile regular expression */
    reti = regcomp(&regex, "adaptive_fmts[ \t]*\":[ \t]*\"[^\"]*", REG_NEWLINE|REG_EXTENDED);
    if( reti )
    {
    	fprintf(stderr, "Could not compile regex\n");
    	return 0;
    }



    /* Execute regular expression */
    reti = regexec(&regex, data, 1, &m, 0);
    if( !reti )
    {
    	*start = m.rm_so ;
    	*finish = m.rm_eo;
    }
    else if( reti == REG_NOMATCH )
    {
    	//puts("No match stream map");
    	return 0;
    }

    else{
    	regerror(reti, &regex, msgbuf, sizeof(msgbuf));
    	fprintf(stderr, "Regex match failed: %s\n", msgbuf);
    	return 0;
    }
#ifdef DEBUG
    printf ("'%.*s' (bytes %d:%d)\n", (*finish - *start),
                        data + *start, *start, *finish);
#endif
//
//	data[*finish]='\0';
//
//#ifdef DEBUG
//    printf ("'%s' (bytes %d:%d)\n", data + *start, *start, *finish);
//#endif
    /* Free compiled regular expression */
    regfree(&regex);
 //   metric.adaptive = true;
    return 1;
}

