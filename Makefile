BUILD_PATH=$(PWD)/build
CC=gcc
CCFLAGS=-std=gnu99 -c -g -DCORO_SJLJ -I$(BUILD_PATH)/libcurl_install/include -I$(BUILD_PATH)/ffmpeg_install/include
LDFLAGS=-L$(BUILD_PATH)/libcurl_install/lib -L$(BUILD_PATH)/ffmpeg_install/lib
LDLIBS=-lcurl -lavformat -lavcodec -lavutil -lm -lssl -lcrypto -lz

FFMPEG_VERSION := 2.2.3
LIBCURL_VERSION := 7.36.0


COMMON_OBJ=src/adaptive.o src/curlops.o src/helper.o src/mm_parser.o src/coro.o src/getinfo.o src/metrics.o 
ALL_SRC=src/adaptive.c src/curlops.c src/helper.c src/mm_parser.c src/coro.c src/getinfo.c src/metrics.c src/youtube-dl.c
ALL_HDR=src/adaptive.h src/coro.h src/getinfo.h src/metrics.h src/youtube-dl.h src/attributes.h src/curlops.h src/helper.h src/mm_parser.h
DEPS=$(BUILD_PATH)/libcurl_install/lib/libcurl.a $(BUILD_PATH)/ffmpeg_install/lib/libavformat.so


all: vdo_client

$(BUILD_PATH)/curl-$(LIBCURL_VERSION).tar.bz2:
	mkdir -p build
	curl -L http://curl.haxx.se/download/curl-$(LIBCURL_VERSION).tar.bz2 -o $@

$(BUILD_PATH)/curl-$(LIBCURL_VERSION)/configure: $(BUILD_PATH)/curl-$(LIBCURL_VERSION).tar.bz2
	test -d $@ || ( /bin/tar -C build -xf $<; touch $@ )
	rm $(BUILD_PATH)/curl-$(LIBCURL_VERSION)/src/tool_hugehelp.c

$(BUILD_PATH)/libcurl/Makefile: $(BUILD_PATH)/curl-$(LIBCURL_VERSION)/configure
	mkdir -p $(BUILD_PATH)/libcurl/
	cd $(BUILD_PATH)/libcurl/; \
	../curl-$(LIBCURL_VERSION)/configure --disable-shared --enable-static --prefix=/ --libdir=/lib --with-ca-bundle=/etc/ca-bundle.crt --enable-ipv6 --disable-manual --disable-dict --disable-file --disable-file --disable-ftp --disable-gopher --disable-imap --disable-pop3 --disable-rtsp --disable-smtp --disable-telnet --disable-tftp

$(BUILD_PATH)/libcurl_install/lib/libcurl.a: $(BUILD_PATH)/libcurl/Makefile
	mkdir -p $(BUILD_PATH)/libcurl_install/
	cd $(BUILD_PATH)/libcurl/; \
	$(MAKE) install DESTDIR=$(BUILD_PATH)/libcurl_install/
	
	
$(BUILD_PATH)/ffmpeg-$(FFMPEG_VERSION).tar.bz2:
	mkdir -p $(BUILD_PATH)
	curl -L http://ffmpeg.org/releases/ffmpeg-$(FFMPEG_VERSION).tar.bz2 -o $@

$(BUILD_PATH)/ffmpeg-$(FFMPEG_VERSION)/configure: $(BUILD_PATH)/ffmpeg-$(FFMPEG_VERSION).tar.bz2
	test -d $@ || ( /bin/tar -C build -xf $<; touch $@ )

$(BUILD_PATH)/ffmpeg/config.h: $(BUILD_PATH)/ffmpeg-$(FFMPEG_VERSION)/configure
	mkdir -p $(BUILD_PATH)/ffmpeg/
	cd $(BUILD_PATH)/ffmpeg/; \
	../ffmpeg-$(FFMPEG_VERSION)/configure --enable-shared --disable-static --prefix=/ --libdir=/lib --disable-all --enable-avformat --enable-avcodec --enable-avutil --enable-demuxer=mov --enable-demuxer=matroska --enable-demuxer=flv --disable-debug --enable-lto --enable-small --disable-zlib --disable-bzlib --disable-pthreads 

$(BUILD_PATH)/ffmpeg_install/lib/libavformat.so: $(BUILD_PATH)/ffmpeg/config.h
	mkdir -p $(BUILD_PATH)/ffmpeg_install/
	cd $(BUILD_PATH)/ffmpeg/; \
	$(MAKE) install DESTDIR=$(BUILD_PATH)/ffmpeg_install/

clean:
	rm vdo_client $(COMMON_OBJ)

.c.o: $(ALL_HDR) $(DEPS)
	$(CC) $(CCFLAGS) $*.c -o $*.o 

vdo_client: $(DEPS) $(COMMON_OBJ) src/youtube-dl.o 
	$(CC) -o vdo_client src/youtube-dl.o $(COMMON_OBJ) $(LDFLAGS) $(LDLIBS)


