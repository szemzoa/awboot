#ifndef __AWBOOT_ASM_CACHE_H__
#define __AWBOOT_ASM_CACHE_H__

#include <types.h>

void flush_dcache_range(unsigned long start, unsigned long stop);
void invalidate_icache_all(void);

#endif
