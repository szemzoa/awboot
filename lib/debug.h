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
#else
#define trace(...)
#endif

#if LOG_LEVEL >= LOG_DEBUG
#define debug(fmt, ...) message("[D] " fmt, ##__VA_ARGS__)
#else
#define debug(...)
#endif

#if LOG_LEVEL >= LOG_INFO
#define info(fmt, ...) message("[I] " fmt, ##__VA_ARGS__)
#else
#define info(...)
#endif

#if LOG_LEVEL >= LOG_WARNING
#define warning(fmt, ...) message("[W] " fmt, ##__VA_ARGS__)
#else
#define warning(...)
#endif

#if LOG_LEVEL >= LOG_ERROR
#define error(fmt, ...) message("[E] " fmt, ##__VA_ARGS__)
#else
#define error(...)
#endif

#define fatal(fmt, ...)                                         \
	{                                                           \
		message("[F] " fmt "restarting...\r\n", ##__VA_ARGS__); \
		mdelay(100);                                            \
		reset();                                                \
	}

void message(const char *fmt, ...);

#endif
