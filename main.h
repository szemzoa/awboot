#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "string.h"
#include "io.h"
#include "types.h"
#include "debug.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#define ALIGN(size, align) (((size) + (align)-1) & (~((align)-1)))
#define OF_ALIGN(size)	   ALIGN(size, 4)

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#ifndef NULL
#define NULL 0
#endif

#define FALSE 0
#define TRUE  1

static inline unsigned int swap_uint32(unsigned int data)
{
	volatile unsigned int a, b, c, d;

	a = ((data)&0xff000000) >> 24;
	b = ((data)&0x00ff0000) >> 8;
	c = ((data)&0x0000ff00) << 8;
	d = ((data)&0x000000ff) << 24;

	return a | b | c | d;
}

#define FILENAME_MAX_LEN 64
typedef struct {
	unsigned int   offset;
	unsigned int   length;
	unsigned char *dest;

	unsigned int   of_offset;
	unsigned char *of_dest;

	char filename[FILENAME_MAX_LEN];
	char of_filename[FILENAME_MAX_LEN];
} image_info_t;

void	 udelay(uint64_t us);
void	 mdelay(uint32_t ms);
void	 sdelay(uint32_t loops);
uint32_t time_ms(void);
uint64_t time_us(void);
uint64_t get_arch_counter(void);
void	 init_sp_irq(uint32_t addr);
void	 reset();

#endif
