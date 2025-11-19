#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdint.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>
#include "string.h"
#include "io.h"
#include "types.h"
#include "debug.h"
#include "ff.h"
#include "sunxi_spi.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#define ALIGN(size, align) (((size) + (align) - 1) & (~((align) - 1)))
#define OF_ALIGN(size)	   ALIGN(size, 4)

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#define MAX_CMD_SIZE	  128
#define MAX_FILENAME_SIZE 32

#ifndef NULL
#define NULL 0
#endif

#define FALSE 0
#define TRUE  1

static inline unsigned int swap_uint32(unsigned int data)
{
	volatile unsigned int a, b, c, d;

	a = ((data) & 0xff000000) >> 24;
	b = ((data) & 0x00ff0000) >> 8;
	c = ((data) & 0x0000ff00) << 8;
	d = ((data) & 0x000000ff) << 24;

	return a | b | c | d;
}

typedef struct {
	unsigned char *kernel_dest;
	unsigned int   kernel_size;

	unsigned char *dtb_dest;
	unsigned int   dtb_size;

	unsigned char *initrd_dest;
	unsigned int   initrd_size;

	char *filename;
	char *of_filename;
	char *initrd_filename;
} image_info_t;

/* Linux zImage Header */
#define LINUX_ZIMAGE_MAGIC 0x016f2818
typedef struct {
	unsigned int code[9];
	unsigned int magic;
	unsigned int start;
	unsigned int end;
} linux_zimage_header_t;

void	 udelay(uint64_t us);
void	 mdelay(uint32_t ms);
void	 sdelay(uint32_t loops);
uint32_t time_ms(void);
uint64_t time_us(void);
uint64_t get_arch_counter(void);
void	 init_sp_irq(uint32_t addr);
void	 reset();

#endif
