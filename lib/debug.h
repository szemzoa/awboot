#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "xformat.h"

#define LOG_ERROR 10
#define LOG_WARN  20
#define LOG_INFO  30
#define LOG_DEBUG 40
#define LOG_TRACE 50

#if LOG_LEVEL >= LOG_TRACE
#define trace(fmt, ...) message("[T] " fmt, ##__VA_ARGS__)
#define UNUSED_TRACE
#else
#define trace(...)
#define UNUSED_TRACE __attribute__((__unused__))
#endif

#if LOG_LEVEL >= LOG_DEBUG
#define debug(fmt, ...) message("[D] " fmt, ##__VA_ARGS__)
#define UNUSED_DEBUG
#else
#define debug(...)
#define UNUSED_DEBUG __attribute__((__unused__))
#endif

#if LOG_LEVEL >= LOG_INFO
#define info(fmt, ...) message("[I] " fmt, ##__VA_ARGS__)
#define UNUSED_INFO
#else
#define info(...)
#define UNUSED_INFO __attribute__((__unused__))
#endif

#if LOG_LEVEL >= LOG_WARNING
#define warning(fmt, ...) message("[W] " fmt, ##__VA_ARGS__)
#define UNUSED_WARNING
#else
#define warning(...)
#define UNUSED_WARNING __attribute__((__unused__))
#endif

#if LOG_LEVEL >= LOG_ERROR
#define error(fmt, ...) message("[E] " fmt, ##__VA_ARGS__)
#define UNUSED_ERROR
#else
#define error(...)
#define UNUSED_ERROR __attribute__((__unused__))
#endif

#define fatal(fmt, ...)                                         \
	{                                                           \
		message("[F] " fmt "restarting...\r\n", ##__VA_ARGS__); \
		mdelay(100);                                            \
		reset();                                                \
	}

void __attribute__((format(printf, 1, 2))) message(const char *fmt, ...);

#endif
