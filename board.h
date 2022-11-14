#ifndef __BOARD_H__
#define __BOARD_H__

#include "dram_v2.h"

#define CONFIG_BOOT_SPINAND
//#define CONFIG_BOOT_SDCARD

#define CONFIG_CPU_FREQ         1200000000

#define CONFIG_ENABLE_CPU_FREQ_DUMP

#define CONFIG_KERNEL_FILENAME	"zImage"
#define CONFIG_DTB_FILENAME	"sun8i-t113-mangopi-dual.dtb"

#define CONFIG_KERNEL_LOAD_ADDR 0x45000000
#define CONFIG_DTB_LOAD_ADDR	0x41800000

#define CONFIG_SPINAND_DTB_ADDR		0x20000
#define CONFIG_SPINAND_KERNEL_ADDR	0x40000

extern dram_para_t ddr_param;
extern struct sunxi_usart_t usart_dbg;
extern struct sunxi_spi_t sunxi_spi0;

extern void board_init(void);
extern int board_sdhci_init(void);

#endif
