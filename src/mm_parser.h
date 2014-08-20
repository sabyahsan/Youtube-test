#ifndef MM_PARSER_H_
#define MM_PARSER_H_

#include <stdint.h>
#include <stddef.h>
#include "coro.h"

extern coro_context corou_main;

void mm_parser(void *arg);

#endif
