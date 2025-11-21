#include <stdint.h>
#include <barrier.h>
#include <asm/cache.h>

static inline void dcache_clean_invalidate_line_by_mva(uintptr_t addr)
{
	__asm__ __volatile__("mcr p15, 0, %0, c7, c14, 1" :: "r"(addr) : "memory");
}

static inline void icache_invalidate_all_cp15(void)
{
	__asm__ __volatile__("mcr p15, 0, %0, c7, c5, 0" :: "r"(0u) : "memory");
}

void flush_dcache_range(unsigned long start, unsigned long stop)
{
	uintptr_t addr;

	for (addr = start; addr < stop; addr += 32) {
		dcache_clean_invalidate_line_by_mva(addr);
	}
	dsb();
}

void invalidate_icache_all(void)
{
	icache_invalidate_all_cp15();
	isb();
}
