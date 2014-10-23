/*
 * curlops.h
 *
 *  Created on: Oct 24, 2013
 *      Author: sahsan
 */

#ifndef CURLOPS_H_
#define CURLOPS_H_

#include "youtube-dl.h"
#include "coro.h"

struct myprogress {
  int stream;
  long lastruntime;
  double lastdlbytes;
  CURL *curl;
  struct {
	  bool init;
	  coro_context parser_coro;
	  struct coro_stack parser_stack;
	  uint8_t *curl_buffer;
	  size_t bytes_avail;
  };
};

int downloadfiles(videourl url [] );
long getfilesize(const char url[]);
int set_ip_version(CURL *curl, enum IPv ip_version);

#endif /* CURLOPS_H_ */
