#ifndef __AWBOOT_ASM_ARMV7_H__
#define __AWBOOT_ASM_ARMV7_H__

#define CPUID_ARM_SEC_SHIFT        4
#define CPUID_ARM_SEC_MASK         (0xF << CPUID_ARM_SEC_SHIFT)
#define CPUID_ARM_VIRT_SHIFT       12
#define CPUID_ARM_VIRT_MASK        (0xF << CPUID_ARM_VIRT_SHIFT)
#define CPUID_ARM_GENTIMER_SHIFT   16
#define CPUID_ARM_GENTIMER_MASK    (0xF << CPUID_ARM_GENTIMER_SHIFT)

#define CBAR_MASK                  0xFFFF8000

#if !defined(__ASSEMBLY__) && !defined(__ASSEMBLER__)
#include <arm32.h>
#include <barrier.h>

int armv7_init_nonsec(void);
void _do_nonsec_entry(void *target_pc, unsigned long r0,
		      unsigned long r1, unsigned long r2);
unsigned int _nonsec_init(void);
void _smp_pen(void);
extern char __secure_start[];
extern char __secure_end[];
extern char __secure_stack_start[];
extern char __secure_stack_end[];

#endif

#endif
