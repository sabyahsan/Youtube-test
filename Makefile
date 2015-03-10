VERSION := 1.0

# _Exit is C99, _exit is POSIX... the wnr3500l is not C99 compliant
wnr3500l_CPPFLAGS := -D_Exit=_exit
wrt54gl_LDLIBS := -lssl -lcrypto -lz

BINARIES := youtube
youtube_SOURCES := $(wildcard src/*.c)
youtube_CPPFLAGS = -D_GNU_SOURCE -DCORO_SJLJ $($(UNIT)_CPPFLAGS)
youtube_CFLAGS := -std=gnu99
youtube_LDLIBS := -lcurl -lavformat -lavcodec -lavutil $($(UNIT)_LDLIBS)

EXTRA_DEPS_wrt54gl := curl

# TODO: Fix this
ifeq ($(EXTRA_DEPS_$(UNIT)),curl)
youtube_DEPS = $(BUILD_PATH)/libcurl_install/lib/libcurl.a

youtube_CPPFLAGS += -I$(BUILD_PATH)/libcurl_install/include
youtube_LDFLAGS = -L$(BUILD_PATH)/libcurl_install/lib

LIBCURL_VERSION := 7.36.0
PRE_RULES := build/curl-$(LIBCURL_VERSION)/configure
endif

youtube_DEPS += $(BUILD_PATH)/ffmpeg_install/lib/libavformat.so

youtube_CPPFLAGS += -I$(BUILD_PATH)/ffmpeg_install/include
youtube_LDFLAGS += -L$(BUILD_PATH)/ffmpeg_install/lib

FFMPEG_VERSION := 2.2.3
PRE_RULES += build/ffmpeg-$(FFMPEG_VERSION)/configure

wnr3500l_FFMPEG_CPU := --arch=mips --enable-mips32r2 --disable-mipsfpu --enable-mipsdspr1 --disable-mipsdspr2
wrt160nl_FFMPEG_CPU := --arch=mips --cpu=24kc
wr741nd_FFMPEG_CPU := --arch=mips --cpu=24kc
wr741ndv4_FFMPEG_CPU := --arch=mips --cpu=24kc
wr1043nd_FFMPEG_CPU := --arch=mips --cpu=24kc
wdr3600_FFMPEG_CPU := --arch=mips --cpu=74kc
wdr4900_FFMPEG_CPU := --arch=ppc --cpu=e500v2

UNSUPPORTED_UNITS := cz alix BT_HomeHub_3_0a

BASE_MAKEFILE ?= /home/sahsan/sk-probe/build_system/Makefile
include $(BASE_MAKEFILE)

build/curl-$(LIBCURL_VERSION).tar.bz2:
	mkdir -p build
	curl -L http://curl.haxx.se/download/curl-$(LIBCURL_VERSION).tar.bz2 -o $@

build/curl-$(LIBCURL_VERSION)/configure: build/curl-$(LIBCURL_VERSION).tar.bz2
	test -d $@ || ( /bin/tar -C build -xf $<; touch $@ )

$(BUILD_PATH)/libcurl/Makefile: build/curl-$(LIBCURL_VERSION)/configure
	mkdir -p $(BUILD_PATH)/libcurl/
	cd $(BUILD_PATH)/libcurl/; \
	export CPPFLAGS='$(DEPS_CPPFLAGS)'; \
	export CFLAGS='$(DEPS_CFLAGS)'; \
	export CXXFLAGS='$(DEPS_CXXFLAGS)'; \
	export LDFLAGS='$(DEPS_LDFLAGS)'; \
	../../curl-$(LIBCURL_VERSION)/configure --disable-shared --enable-static --host=$(patsubst %-,%,$(CROSS)) --prefix=/ --libdir=/lib --with-ca-bundle=/etc/ca-bundle.crt --enable-ipv6 --disable-manual --disable-dict --disable-file --disable-file --disable-ftp --disable-gopher --disable-imap --disable-pop3 --disable-rtsp --disable-smtp --disable-telnet --disable-tftp

$(BUILD_PATH)/libcurl_install/lib/libcurl.a: $(BUILD_PATH)/libcurl/Makefile
	mkdir -p $(BUILD_PATH)/libcurl_install/
	cd $(BUILD_PATH)/libcurl/; \
	$(MAKE) install DESTDIR=${PWD}/$(BUILD_PATH)/libcurl_install/

build/ffmpeg-$(FFMPEG_VERSION).tar.bz2:
	mkdir -p build
	curl -L http://ffmpeg.org/releases/ffmpeg-$(FFMPEG_VERSION).tar.bz2 -o $@

build/ffmpeg-$(FFMPEG_VERSION)/configure: build/ffmpeg-$(FFMPEG_VERSION).tar.bz2
	test -d $@ || ( /bin/tar -C build -xf $<; touch $@ )

$(BUILD_PATH)/ffmpeg/config.h: build/ffmpeg-$(FFMPEG_VERSION)/configure
	mkdir -p $(BUILD_PATH)/ffmpeg/
	cd $(BUILD_PATH)/ffmpeg/; \
	export CPPFLAGS='$(DEPS_CPPFLAGS)'; \
	export CFLAGS='$(DEPS_CFLAGS)'; \
	export CXXFLAGS='$(DEPS_CXXFLAGS)'; \
	export LDFLAGS='$(DEPS_LDFLAGS) -fPIC'; \
	../../ffmpeg-$(FFMPEG_VERSION)/configure --enable-shared --disable-static --prefix=/ --libdir=/lib --disable-all --enable-avformat --enable-avcodec --enable-avutil --enable-demuxer=mov --enable-demuxer=matroska --enable-demuxer=flv --disable-debug --enable-lto --enable-small --disable-zlib --disable-bzlib --disable-pthreads --enable-cross-compile --cross-prefix=$(CROSS) $($(UNIT)_FFMPEG_CPU) --target-os=linux --disable-symver --disable-runtime-cpudetect

$(BUILD_PATH)/ffmpeg_install/lib/libavformat.so: $(BUILD_PATH)/ffmpeg/config.h
	mkdir -p $(BUILD_PATH)/ffmpeg_install/
	cd $(BUILD_PATH)/ffmpeg/; \
	$(MAKE) install DESTDIR=${PWD}/$(BUILD_PATH)/ffmpeg_install/
