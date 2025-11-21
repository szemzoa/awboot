#include <stdint.h>
#include "sunxi_security.h"
#include <psci.h>

static void enable_smp_via_cp15(void)
{
	uint32_t value;

	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 1" : "=r"(value));
	value |= (1u << 6);
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 1" :: "r"(value) : "memory");
	__asm__ __volatile__("isb" ::: "memory");
}

void psci_board_init(void)
{
	sunxi_security_setup();
	enable_smp_via_cp15();
	sunxi_gic_set_nonsecure();
}
