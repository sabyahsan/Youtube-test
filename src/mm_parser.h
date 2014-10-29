#ifndef MM_PARSER_H_
#define MM_PARSER_H_

#include <stdint.h>
#include <stddef.h>
#include <sys/time.h>
#include "coro.h"
#include "helper.h"

extern coro_context corou_main;

void mm_parser(void *arg);
struct timeval get_curr_playoutbuf_len_forstream(int i);
struct timeval get_curr_playoutbuf_len();

#endif
