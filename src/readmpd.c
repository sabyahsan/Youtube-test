//
//  readmpd.c
//  
//
//  Created by Saba Ahsan on 08/11/16.
//
//

#include "readmpd.h"

extern metrics metric;

//
//static void findformat(char * data)
//{
//    int index_adaptive=0, index_nonadaptive=0;
//    
//    /*get the url lists populated*/
//    videourl * url_list = extract_url_list(data, &index_nonadaptive, false);
//    videourl * url_list_adaptive =extract_url_list(data, &index_adaptive, true);
//    
//    for(int i =0 ; i<index_nonadaptive; ++i) {
//        url_list[i].bitrate = getbitrate(url_list[i].url);
//    }
//    
//    memcpy(metric.no_adap_url, url_list, index_nonadaptive * sizeof(*url_list));
//    memset(&metric.no_adap_url[index_nonadaptive], 0, sizeof(*metric.no_adap_url));
//    
//    int a_index = 0;
//    int v_index = 0;
//    for(int i = 0; i < index_adaptive; ++i) {
//        if(strstr(url_list_adaptive[i].type, "audio")) {
//            metric.adap_audiourl[a_index++] = url_list_adaptive[i];
//        } if(strstr(url_list_adaptive[i].type, "video")) {
//            metric.adap_videourl[v_index++] = url_list_adaptive[i];
//        }
//    }
//    memset(&metric.adap_audiourl[a_index], 0, sizeof(*metric.adap_audiourl));
//    memset(&metric.adap_videourl[v_index], 0, sizeof(*metric.adap_videourl));
//    
//    if(url_list!=NULL)
//        free(url_list);
//    if(url_list_adaptive!=NULL)
//        free(url_list_adaptive);
//    
//    qsort(metric.no_adap_url, index_nonadaptive, sizeof(*metric.no_adap_url), compar_bitrate);
//    qsort(metric.adap_audiourl, a_index, sizeof(*metric.adap_audiourl), compar_bitrate);
//    qsort(metric.adap_videourl, v_index, sizeof(*metric.adap_videourl), compar_bitrate);
//}

float get_duration(char * dur )
{
    float hour, min, sec;
    char * tmp2, *tmp3;
    tmp3=dur;
    tmp2 = strstr(tmp3, "H");
    if(tmp2==NULL)
    hour=0.0;
    else
    {
        *tmp2='\0';
        hour = atof(tmp3+2);
        tmp3 = tmp2+1;
    }
    
    tmp2 = strstr(tmp3, "M");
    if(tmp2==NULL)
    min=0.0;
    else
    {
        *tmp2='\0';
        min = atof(tmp3);
        tmp3 = tmp2+1;
    }
    
    tmp2 = strstr(tmp3, "S");
    if(tmp2==NULL)
    sec=0.0;
    else
    {
        *tmp2='\0';
        sec = atof(tmp3);
        tmp3 = tmp2+1;
    }
    return (hour*3600)+(min*60)+sec;
}


/*return length of the new link or 0 if unsuccessful*/

int get_base_url (char * link)
{
    for ( int i = strlen(link) - 1; i >= 0; i --)
    {
        if ( link[i] == '/' )
        {
            link[i+1] = '\0';
            return strlen(link);
        }
    }
    return 0;
}


int read_mpddata(char * memory, char mpdlink[])
{
    int             v_index = 0;
    xmlDoc          *document;
    xmlNode         *root, *first_child, *node, *second_child, *node2, *third_child, *node3;
    char            *filename;
    char            duration[25]="\0";
    xmlAttr         * attribute;
    float           dur, segdur=0, timescale=0;
    int             num_of_rates = 0, height;
    char            bw[25];
    char            bandwidth[MAX_SUPPORTED_BITRATE_LEVELS][25];
    char *          tmp, * newurl;
    char            keyword_bw [12] = "$Bandwidth$";
    char            keyword_num [12] = "$Number$";
    char            segnum[5];
    char            init_url[MAXURLLENGTH];
    char            media_url[MAXURLLENGTH];
    
    
    
    strcpy(init_url, mpdlink);
    if(get_base_url(init_url)==0)
    {
        printf("Unable to successfully extract the base URL\n");
        return -1;
    }
    strcpy(media_url, init_url);
        
    document = xmlReadMemory(memory, strlen(memory), mpdlink, NULL, 0);
    root = xmlDocGetRootElement(document);
    //fprintf(stdout, "Root is <%s> (%i)\n", root->name, root->type);
    memset(&metric.adap_videourl[v_index], 0, sizeof(*metric.adap_videourl));

    
    first_child = root->children;
    for (node = first_child; node; node = node->next)
    {
        //fprintf(stdout, "\t Child is <%s> (%i)\n", node->name, node->type);
        if(xmlStrcmp(node->name, (const xmlChar *) "Period")==0)
        {
            attribute = node->properties;
            while(attribute)
            {
                if(xmlStrcmp(attribute->name, (const xmlChar *) "duration")==0)
                {
                    xmlChar* value = xmlNodeListGetString(node->doc, attribute->children, 1);
                    //printf("\t\t%s : %s\n",(char *)attribute->name, (char *)value);
                    strcpy(duration, (char *)value);
                    printf("Duration is %s and %f\n", duration, dur =get_duration(duration));
                    get_duration(duration);
                    xmlFree(value);
                    break;
                }
                attribute = attribute->next;
            }
            second_child = node->children;
            for (node2 = second_child; node2; node2 = node2->next)
            {
                //fprintf(stdout, "\t\t Child is <%s> (%i)\n", node2->name, node2->type);
                if(xmlStrcmp(node2->name, (const xmlChar *) "AdaptationSet")==0)
                {
                    third_child = node2->children;
                    for (node3 = third_child; node3; node3 = node3->next)
                    {
                        if(xmlStrcmp(node3->name, (const xmlChar *) "SegmentTemplate")==0)
                        {
                            attribute = node3->properties;
                            while(attribute)
                            {
                                //printf(">>>>>>>>>>>>>>>>%s\n", (char *)attribute->name);
                                if(xmlStrcmp(attribute->name, (const xmlChar *) "duration")==0)
                                {
                                    xmlChar* value = xmlNodeListGetString(node->doc, attribute->children, 1);
                                    segdur=atoi((char *)value);
                                    xmlFree(value);
                                }
                                else if(xmlStrcmp(attribute->name, (const xmlChar *) "timescale")==0)
                                {
                                    xmlChar* value = xmlNodeListGetString(node->doc, attribute->children, 1);
                                    timescale=atoi((char *)value);
                                    xmlFree(value);
                                }
                                else if(xmlStrcmp(attribute->name, (const xmlChar *) "initialization")==0)
                                {
                                    xmlChar* value = xmlNodeListGetString(node->doc, attribute->children, 1);
                                    strcat(init_url, (char *)value);
                                    //printf("Init_url being filled here!!! %s (%s)\n", init_url, (char *)value);
                                    xmlFree(value);
                                }
                                else if(xmlStrcmp(attribute->name, (const xmlChar *) "media")==0)
                                {
                                    xmlChar* value = xmlNodeListGetString(node->doc, attribute->children, 1);
                                    strcat(media_url, (char *) value);
                                    //printf("Media_url is %s \n", media_url);
                                    xmlFree(value);
                                }
                                
                                attribute = attribute->next;
                            }
                            /* add 2 to include init segment*/
                            metric.num_of_segments = ceil(dur/(segdur/timescale)) + 1;
                            //printf("Init segment : %s\n", init_url);
                            //printf("Timescale : %f, Seg duration : %f\n", timescale, segdur);
                            //printf("Number of Segments = %d\n", metric.num_of_segments);
                        }
                        
                        if(xmlStrcmp(node3->name, (const xmlChar *) "Representation")==0)
                        {
                            attribute = node3->properties;
                            while(attribute)
                            {
                                if(xmlStrcmp(attribute->name, (const xmlChar *) "height")==0)
                                {
                                    xmlChar* value = xmlNodeListGetString(node->doc, attribute->children, 1);
                                    height=atoi((char *)value);
                                    xmlFree(value);
                                }
                                else if(xmlStrcmp(attribute->name, (const xmlChar *) "bandwidth")==0)
                                {
                                    xmlChar* value = xmlNodeListGetString(node->doc, attribute->children, 1);
                                    strcpy(bw,(char *)value);
                                    xmlFree(value);
                                }
                                
                                attribute = attribute->next;
                            }

                            strcpy(bandwidth[num_of_rates], bw);
                            num_of_rates++;
                            if(num_of_rates>=MAX_SUPPORTED_BITRATE_LEVELS)
                            {
                                printf("Number of rate levels exceeds the maximum allowed value\n");
                                return -1;
                            }
                            
                        }
                        
                    }
                    
                }
            }
            
        }

    }

    if( metric.num_of_segments < 0)
    {
        printf("Number of rate levels / segments is negative, check mpd. \n");
        return -1;
    }
    
    metric.num_of_levels = num_of_rates;
    
    for (int j = 0; j < num_of_rates; j++)
    {
        manifest * next_level = &metric.bitrate_level[j];
        next_level->segments = malloc (metric.num_of_segments * sizeof(char *));
        
        for (int k = 0; k < metric.num_of_segments ; k++)
            next_level->segments[k] = malloc ( MAXURLLENGTH * sizeof (char));
        
        next_level->bitrate = atoi(bandwidth[j]);

        newurl = str_replace(init_url, keyword_bw, bandwidth[j]);
        strcpy(next_level->segments[0], newurl);
        free(newurl);

        for (int k = 1; k < metric.num_of_segments; k++)
        {
            sprintf(segnum,"%d", k);
            tmp = str_replace(media_url, keyword_bw, bandwidth[j]);
            newurl = str_replace(tmp, keyword_num, segnum);
            strcpy(next_level->segments[k], newurl);
            //printf("%s\n", next_level->segments[k]);
            free(tmp);
            free(newurl);
        }
    }
    
    return 0;

    
}
