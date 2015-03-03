#ifndef EXCEPTION_H_
#define EXCEPTION_H_

#include <string.h>
#include <stdarg.h>

#define MAXLEN_ERR_MSG 1024

enum errors {
	ITWORKED = 600,
	BADFORMATS = 601, //we didn't find the right format for download.
	RUNTIME_ERROR = 602,
	MAXDOWNLOADLENGTH = 603, /*three minutes of video has been downloaded*/
	MAXTESTRUNTIME = 604, /*test has been downloading video for the maximum time */
	SIGNALHIT = 605, /* program received ctrl+c*/
	CODETROUBLE = 606, /*HTTP code couldn't be retrieved*/
	ERROR_STALL = 607, /* There was a stall */
	TOO_FAST = 608, /* The test finsihed too fast */
	PARSERROR = 609, /*There was an error with the initial HTTP response*/
	CURLERROR = 610,
	CURLERROR_GETINFO = 611,
	BAD_PARAMETERS = 612,
	STALLED_ON_FINAL = 613,
	STALLED_WILL_STEP_DOWN = 614,
	NETWORK_API_ERROR = 615,
	DNS_RESOLUTION_API_ERROR = 616,
	CONNECTION_API_ERROR = 617,
	NETWORK_CONTENT_ERROR = 618,
	DNS_RESOLUTION_CONTENT_ERROR = 619,
	FIRSTRESPONSERROR = 620,
	CONNECTION_CONTENT_ERROR = 621,
	WERSCREWED = 666
};

#if 0
/* 600 offset */
static const char* const error_msgs[] = {
		[0] = "ITWORKED",
		[1] = "BADFORMATS",
		[2] = "RUNTIME_ERROR",
		[3] = "MAXDOWNLOADLENGTH",
		[4] = "MAXTESTRUNTIME",
		[5] = "SIGNALHIT",
		[6] = "CODETROUBLE",
		[7] = "ERROR_STALL",
		[8] = "TOO_FAST",
		[9] = "PARSERROR",
		[10] = "CURLERROR",
		[11] = "CURLERROR_GETINFO",
		[12] = "BAD_PARAMETERS",
		[13] = "STALLED_ON_FINAL",
		[14] = "STALLED_WILL_STEP_DOWN",
		[15] = "NETWORK_API_ERROR",
		[16] = "DNS_RESOLUTION_API_ERROR",
		[17] = "CONNECTION_API_ERROR",
		[18] = "NETWORK_CONTENT_ERROR",
		[19] = "DNS_RESOLUTION_CONTENT_ERROR",
		[20] = "FIRSTRESPONSERROR",
		[21] = "CONNECTION_CONTENT_ERROR",
		[66] = "WERSCREWED"
};
#endif

struct appexception {
	enum errors error;
	char msg[MAXLEN_ERR_MSG];
};

struct appexception exception;

extern metrics metric;

static inline void create_exception(enum errors error, const char *msg, ...) {
	//exception.error = error;
	metric.errorcode = error;

	va_list ap;
	va_start(ap, msg);
	vsnprintf(exception.msg, sizeof(exception.msg), msg, ap);
	va_end(ap);
}

static inline bool is_exception() {
	return metric.errorcode != 0;
}

#endif
