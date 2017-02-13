CC=gcc
CCFLAGS=-c -g -D_GNU_SOURCE -DCORO_SJLJ -I/usr/local/include `xml2-config --cflags`
LDFLAGS=-L/usr/local/lib -lcurl -lavformat -lavcodec -lavutil `xml2-config --libs`

COMMON_OBJ=src/adaptive.o src/curlops.o src/helper.o src/mm_parser.o src/coro.o src/metrics.o src/readmpd.o
ALL_SRC=src/adaptive.c src/curlops.c src/helper.c src/mm_parser.c src/coro.c src/metrics.c src/youtube-dl.c src/readmpd.c
ALL_HDR=src/adaptive.h src/coro.h src/metrics.h src/youtube-dl.h src/attributes.h src/curlops.h src/helper.h src/mm_parser.h src/readmpd.h

all: vdo_client

clean:
	rm vdo_client $(COMMON_OBJ)

.c.o: $(ALL_HDR)
	$(CC) $(CCFLAGS) $*.c -o $*.o 

vdo_client: src/youtube-dl.o $(COMMON_OBJ)
	$(CC) -lm -o vdo_client src/youtube-dl.o $(COMMON_OBJ) $(LDFLAGS) 


