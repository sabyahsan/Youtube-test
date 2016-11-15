#include "mm_parser.h"
#include "metrics.h"
#include "youtube-dl.h"
#include "coro.h"
#include "attributes.h"
#include "curlops.h"
#include <stdint.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

extern metrics metric;

#define DIV_ROUND_CLOSEST(x, divisor)(                  \
{                                                       \
        typeof(divisor) __divisor = divisor;            \
        (((x) + ((__divisor) / 2)) / (__divisor));      \
}                                                       \
)

#define SEC2PICO UINT64_C(1000000000000)
//#define SEC2NANO 1000000000
#define SEC2MILI 1000

coro_context corou_main;

static int read_packet(void *opaque, uint8_t *buf, int buf_size) {
	struct myprogress *prog = ((struct myprogress *) opaque);

	coro_transfer(&prog->parser_coro, &corou_main);

	assert(prog->bytes_avail <= (size_t)buf_size);

	memcpy(buf, prog->curl_buffer, prog->bytes_avail);

	return prog->bytes_avail;
}

void mm_parser(void *arg) {
	struct myprogress *prog = ((struct myprogress *) arg);

	void *buff = av_malloc(CURL_MAX_WRITE_SIZE);
	AVIOContext *avio = avio_alloc_context(buff, CURL_MAX_WRITE_SIZE, 0,
			prog, read_packet, NULL, NULL);
	if (avio == NULL) {
		exit(EXIT_FAILURE);
	}

	AVFormatContext *fmt_ctx = avformat_alloc_context();
	if (fmt_ctx == NULL) {
		exit(EXIT_FAILURE);
	}
	fmt_ctx->pb = avio;

	int ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
	if (ret < 0) {
		exit(EXIT_FAILURE);
	}

	avformat_find_stream_info(fmt_ctx, NULL);

	int videoStreamIdx = -1;
	int audioStreamIdx = -1;
	for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
		if (fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStreamIdx = i;
		} else if (fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioStreamIdx = i;
		}
	}

	unsigned long long vtb = 0;
	if (videoStreamIdx != -1) {
		int vnum = fmt_ctx->streams[videoStreamIdx]->time_base.num;
		if (vnum > (int) (UINT64_MAX / SEC2PICO)) {
			exit(EXIT_FAILURE);
		}
		int vden = fmt_ctx->streams[videoStreamIdx]->time_base.den;
		vtb = DIV_ROUND_CLOSEST(vnum * SEC2PICO, vden);
	}

	unsigned long long atb = 0;
	if (audioStreamIdx != -1) {
		int anum = fmt_ctx->streams[audioStreamIdx]->time_base.num;
		if (anum > (int) (UINT64_MAX / SEC2PICO)) {
			exit(EXIT_FAILURE);
		}
		int aden = fmt_ctx->streams[audioStreamIdx]->time_base.den;
		atb = DIV_ROUND_CLOSEST(anum * SEC2PICO, aden);
	}

	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;
	while (av_read_frame(fmt_ctx, &pkt) >= 0) {
		if (pkt.stream_index == videoStreamIdx) {
			if (pkt.dts > 0) {
                /*SA-102014. metric.TSnow is the overall timestamp for which both audio and video packets have been received,
                it should not be changed for video frames only. This would not affect the flow when the video is received
                slower than the audio, but if it's the other way around, then the results would be inaccurate
                metric.TSnow = (pkt.dts * vtb) / (SEC2PICO / SEC2MILI);*/
				metric.TSlist[STREAM_VIDEO] = (pkt.dts * vtb) / (SEC2PICO / SEC2MILI);
				/*SA-10214- checkstall should be called after the TS is updated for each stream, instead of when new packets 
				  arrive, this ensures that we know exactly what time the playout would stop and stall would occur*/ 
				checkstall(false); 
			}
		} else if (pkt.stream_index == audioStreamIdx) {
			if (pkt.dts > 0) {
				metric.TSlist[STREAM_AUDIO] = (pkt.dts * atb) / (SEC2PICO / SEC2MILI);
				/*SA-10214- checkstall should be called after the TS is updated for each stream, instead of when new packets 
				  arrive, this ensures that we know exactly what time the playout would stop and stall would occur*/ 
				checkstall(false); 
			}
		}
		av_free_packet(&pkt);
	}

	avformat_free_context(fmt_ctx);
	av_free(avio);

	return;
}

/*If the function returns a negative value, it means there is space in the buffer, otherwise it indicates 
the over utilization of the buffer in seconds*/
struct timeval get_curr_playoutbuf_len()
{
	struct timeval buffertime; 
	/*check if playout is stalled for any reason, return full length of buffer if so. */
	if(metric.Tmin<0)
	{
		buffertime.tv_sec=0; 
		buffertime.tv_usec=0; 
	}
	else
	{
		long long timenow = gettimelong();
		double leninusec =  ((((double)(metric.TSnow-metric.TS0)*1000)-(double)(timenow-metric.Tmin)));
 		buffertime.tv_sec = leninusec/1000000;
		buffertime.tv_usec = leninusec - (buffertime.tv_sec*1000000);
	}
	return buffertime;
}


struct timeval get_curr_playoutbuf_len_forstream(int i)
{
	/*check if playout is stalled for any reason, return full length of buffer if so. */
	struct timeval buffertime; 
	/*check if playout is stalled for any reason, return full length of buffer if so. */
	if(metric.Tmin<0)
	{
		buffertime.tv_sec=0; 
		buffertime.tv_usec=0; 
	}
	else
	{
		long long timenow = gettimelong();
		double leninusec =  ((((double)(metric.TSlist[i]-metric.TS0)*1000)-(double)(timenow-metric.Tmin)));
 		buffertime.tv_sec = (long)(leninusec/1000000);
		buffertime.tv_usec = leninusec - (buffertime.tv_sec*1000000);
	}
	return buffertime;
}

