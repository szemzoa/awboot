#ifndef __ARM32_H__
#define __ARM32_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

static inline uint32_t arm32_read_p15_c1(void)
{
	uint32_t value;

	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0" : "=r"(value) : : "memory");

	return value;
}

static inline void arm32_write_p15_c1(uint32_t value)
{
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 0" : : "r"(value) : "memory");
	arm32_read_p15_c1();
}

static inline void arm32_interrupt_enable(void)
{
	uint32_t tmp;

	__asm__ __volatile__("mrs %0, cpsr\n"
						 "bic %0, %0, #(1<<7)\n"
						 "msr cpsr_cxsf, %0"
						 : "=r"(tmp)
						 :
						 : "memory");
}

static inline void arm32_interrupt_disable(void)
{
	uint32_t tmp;

	__asm__ __volatile__("mrs %0, cpsr\n"
						 "orr %0, %0, #(1<<7)\n"
						 "msr cpsr_cxsf, %0"
						 : "=r"(tmp)
						 :
						 : "memory");
}

static inline void arm32_mmu_enable(void)
{
	uint32_t value = arm32_read_p15_c1();
	arm32_write_p15_c1(value | (1 << 0));
}

static inline void arm32_mmu_disable(void)
{
	uint32_t value = arm32_read_p15_c1();
	arm32_write_p15_c1(value & ~(1 << 0));
}

static inline void arm32_dcache_enable(void)
{
	uint32_t value = arm32_read_p15_c1();
	arm32_write_p15_c1(value | (1 << 2));
}

static inline void arm32_dcache_disable(void)
{
	uint32_t value = arm32_read_p15_c1();
	arm32_write_p15_c1(value & ~(1 << 2));
}

static inline void arm32_icache_enable(void)
{
	uint32_t value = arm32_read_p15_c1();
	arm32_write_p15_c1(value | (1 << 12));
}

static inline void arm32_icache_disable(void)
{
	uint32_t value = arm32_read_p15_c1();
	arm32_write_p15_c1(value & ~(1 << 12));
}

#ifdef __cplusplus
}
#endif

#endif /* __ARM32_H__ */
