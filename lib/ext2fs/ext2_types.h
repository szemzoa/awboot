#ifndef __EXT2_TYPES_H__
#define __EXT2_TYPES_H__

#include <inttypes.h>
#include <stddef.h>
#include "endian.h"

typedef uintptr_t	 addr_t;
typedef long		 ssize_t;
typedef int			 status_t;
typedef uint64_t	 off_t;
typedef unsigned int uint;

#include "nutheap.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

extern int atomic_add(volatile int *ptr, int val);

/* arm specific stuff */
#define PAGE_SIZE  4096
#define CACHE_LINE 64

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define ROUNDUP(a, b)	(((a) + ((b)-1)) & ~((b)-1))
#define ROUNDDOWN(a, b) ((a) & ~((b)-1))

/* Macro returns UINT_MAX in case of overflow */
#define ROUND_TO_PAGE(x, y) ((ROUNDUP((x), ((y) + 1)) < (x)) ? UINT_MAX : ROUNDUP((x), ((y) + 1)))

/* allocate a buffer on the stack aligned and padded to the cpu's cache line size */
#define STACKBUF_DMA_ALIGN(var, size)      \
	uint8_t	 __##var[(size) + CACHE_LINE]; \
	uint8_t *var = (uint8_t *)(ROUNDUP((addr_t)__##var, CACHE_LINE))

/* Macro to allocate buffer in both local & global space, the STACKBUF_DMA_ALIGN cannot
 * be used for global space.
 * If we use STACKBUF_DMA_ALIGN 'C' compiler throws the error "Initializer element
 * is not constant", since global variable need to be initialized with a constant value.
 */
#define BUF_DMA_ALIGN(var, size) static uint8_t var[ROUNDUP(size, CACHE_LINE)] __attribute__((aligned(CACHE_LINE)));

// define a macro that unconditionally swaps
#define SWAP_64(x)                                                                                                     \
	((((x)&0xff00000000000000ull) >> 56) | (((x)&0x00ff000000000000ull) >> 40) | (((x)&0x0000ff0000000000ull) >> 24) | \
	 (((x)&0x000000ff00000000ull) >> 8) | (((x)&0x00000000ff000000ull) << 8) | (((x)&0x0000000000ff0000ull) << 24) |   \
	 (((x)&0x000000000000ff00ull) << 40) | (((x)&0x00000000000000ffull) << 56))
#define SWAP_32(x) \
	(((uint32_t)(x) << 24) | (((uint32_t)(x)&0xff00) << 8) | (((uint32_t)(x)&0x00ff0000) >> 8) | ((uint32_t)(x) >> 24))
#define SWAP_16(x) ((((uint16_t)(x)&0xff) << 8) | ((uint16_t)(x) >> 8))

// standard swap macros
#if !defined(__LITTLE_ENDIAN)
#define LE64(val) SWAP_64(val)
#define LE32(val) SWAP_32(val)
#define LE16(val) SWAP_16(val)
#define BE64(val) (val)
#define BE32(val) (val)
#define BE16(val) (val)
#else
#define LE64(val) (val)
#define LE32(val) (val)
#define LE16(val) (val)
#define BE64(val) SWAP_64(val)
#define BE32(val) SWAP_32(val)
#define BE16(val) SWAP_16(val)
#endif

#define LE64SWAP(var) (var) = LE64(var);
#define LE32SWAP(var) (var) = LE32(var);
#define LE16SWAP(var) (var) = LE16(var);
#define BE64SWAP(var) (var) = BE64(var);
#define BE32SWAP(var) (var) = BE32(var);
#define BE16SWAP(var) (var) = BE16(var);

#endif
