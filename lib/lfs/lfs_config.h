#ifndef LFS_CONFIG_H
#define LFS_CONFIG_H

// Config for bootloader only, not mklittlefs

#include <stddef.h>
#include "debug.h"

#define LFS_NO_MALLOC 1
#define LFS_READONLY 1
// #define LFS_NO_ASSERT 1
// #define LFS_NO_DEBUG 1

#ifndef LFS_NO_ASSERT
#define LFS_ASSERT(A) if (!(A)) debug("%s:%d:assert failed: %s \r\n", __FILE__, __LINE__, #A)
#endif // LFS_NO_ASSERT

// #define LFS_TRACE_(fmt, ...) \
//     debug("%s:%d:trace: " fmt "%s\r\n", __FILE__, __LINE__, __VA_ARGS__)
// #define LFS_TRACE(...) LFS_TRACE_(__VA_ARGS__, "")

#define LFS_DEBUG_(fmt, ...) \
    debug("%s:%d:debug: " fmt "%s\r\n", __FILE__, __LINE__, __VA_ARGS__)
#define LFS_DEBUG(...) LFS_DEBUG_(__VA_ARGS__, "")

#define LFS_WARN_(fmt, ...) \
    debug("%s:%d:warn: " fmt "%s\r\n", __FILE__, __LINE__, __VA_ARGS__)
#define LFS_WARN(...) LFS_WARN_(__VA_ARGS__, "")

#define LFS_ERROR_(fmt, ...) \
    debug("%s:%d:error: " fmt "%s\r\n", __FILE__, __LINE__, __VA_ARGS__)
#define LFS_ERROR(...) LFS_ERROR_(__VA_ARGS__, "")

#endif
