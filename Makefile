CC=gcc
CCFLAGS=-c -g -DCORO_SJLJ -I/usr/local/include
LDFLAGS=-L/usr/local/lib -lcurl -lavformat -lavcodec -lavutil

COMMON_OBJ=src/adaptive.o src/curlops.o src/helper.o src/mm_parser.o src/coro.o src/getinfo.o src/metrics.o 
ALL_SRC=src/adaptive.c src/curlops.c src/helper.c src/mm_parser.c src/coro.c src/getinfo.c src/metrics.c src/youtube-dl.c
ALL_HDR=src/adaptive.h src/coro.h src/getinfo.h src/metrics.h src/youtube-dl.h src/attributes.h src/curlops.h src/helper.h src/mm_parser.h

all: vdo_client

clean:
	rm vdo_client $(COMMON_OBJ)

.c.o: $(ALL_HDR)
	$(CC) $(CCFLAGS) $*.c -o $*.o 

vdo_client: src/youtube-dl.o $(COMMON_OBJ)
	$(CC) -lm -o vdo_client src/youtube-dl.o $(COMMON_OBJ) $(LDFLAGS) 


