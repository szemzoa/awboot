#ifndef __T113_REG_R_CCU_H__
#define __T113_REG_R_CCU_H__

#include "types.h"

#define T113_R_CCU_BASE (0x07010000)

#define R_CCU_BIT_RST BIT(0)
#define R_CCU_BIT_EN  BIT(16)

/* PRCM Register List */
#define CPUS_CFG		 0x0000
#define CPUS_APBS1_CFG	 0x000C
#define CPUS_APBS2_CFG	 0x0010
#define CPUS_TIMER_GATE	 0x011C
#define CPUS_PWM_GATE	 0x013C
#define CPUS_UART_GATE	 0x018C
#define CPUS_TWI_GATE	 0x019C
#define CPUS_RSB_GATE	 0x01BC
#define CPUS_CIR_CFG	 0x01C0
#define CPUS_CIR_GATE	 0x01CC
#define CPUS_OWC_CFG	 0x01E0
#define CPUS_OWC_GATE	 0x01EC
#define CPUS_RTC_GATE	 0x020C
#define CPUS_CLK_MAX_REG 0x020C

#define CLK_BUS_R_TIMER	 0x11c
#define CLK_BUS_R_TWD	 0x12c
#define CLK_BUS_R_PPU	 0x1ac
#define CLK_BUS_R_IR_RX	 0x1cc
#define CLK_BUS_R_RTC	 0x20c
#define CLK_BUS_R_CPUCFG 0x22c

#endif /* __T113_REG_CCU_H__ */
