/*
 * (C) Copyright 2018-2020
 * SPDX-License-Identifier:	GPL-2.0+
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangwei <wangwei@allwinnertech.com>
 *
 */

#include "main.h"
#include "io.h"

/*
 * 64bit arch timer.CNTPCT
 * Freq = 24000000Hz
 */
static inline uint64_t get_arch_counter(void)
{
	uint32_t low = 0, high = 0;
	asm volatile("mrrc p15, 0, %0, %1, c14" : "=r"(low), "=r"(high) : : "memory");
	return (((uint64_t)high) << 32 | low);
}

/*
 * get current time.(millisecond)
 */
uint32_t get_sys_ticks(void)
{
	return (uint32_t)get_arch_counter() / 24000;
}

void udelay(unsigned long us)
{
	uint64_t t1, t2;

	t1 = get_arch_counter();
	t2 = t1 + us * 24;
	do {
		t1 = get_arch_counter();
	} while (t2 >= t1);
}

void mdelay(unsigned long ms)
{
	udelay(ms * 1000);
}

/************************************************************
 * sdelay() - simple spin loop.  Will be constant time as
 *  its generally used in bypass conditions only.  This
 *  is necessary until timers are accessible.
 *
 *  not inline to increase chances its in cache when called
 *************************************************************/
void sdelay(unsigned long loops)
{
	__asm__ volatile("1:\n"
					 "subs %0, %1, #1\n"
					 "bne 1b"
					 : "=r"(loops)
					 : "0"(loops));
}
