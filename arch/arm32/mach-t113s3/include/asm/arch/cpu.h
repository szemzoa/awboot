#ifndef __AWBOOT_ASM_ARCH_CPU_H__
#define __AWBOOT_ASM_ARCH_CPU_H__

#include "config.h"

#define SUNXI_GIC400_BASE       CONFIG_ARM_GIC_BASE_ADDRESS
#define SUNXI_SRAMC_BASE        0x07000000u
#define SUNXI_PRCM_BASE         0x07010000u
#define SUNXI_R_PRCM_BASE       0x07010000u
#define SUNXI_R_CPUCFG_BASE     0x07000400u
#define SUNXI_CPUCFG_BASE       0x09010000u

#endif
