#ifndef __BOARD_H__
#define __BOARD_H__

#include "dram_v2.h"

#define CONFIG_CPU_FREQ         1200000000

#define CONFIG_ENABLE_CPU_FREQ_DUMP

#define CONFIG_KERNEL_FILENAME	"zImage"
#define CONFIG_DTB_FILENAME	"sun8i-mangopi-mq-dual-linux.dtb"

#define CONFIG_KERNEL_LOAD_ADDR 0x45000000
#define CONFIG_DTB_LOAD_ADDR	0x41800000

extern dram_para_t ddr_param;
extern struct sunxi_usart_t usart_dbg;

extern void board_init(void);
extern int board_sdhci_init(void);

#endif
