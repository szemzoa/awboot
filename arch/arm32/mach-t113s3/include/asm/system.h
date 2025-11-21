#ifndef __AWBOOT_ASM_SYSTEM_H__
#define __AWBOOT_ASM_SYSTEM_H__

#include <barrier.h>

#ifndef wfi
#define wfi() __asm__ __volatile__("wfi")
#endif

#endif
