#ifndef __BOARD_MANGOPI_DUAL_H__
#define __BOARD_MANGOPI_DUAL_H__

#include "dram.h"
#include "sunxi_spi.h"
#include "sunxi_usart.h"
#include "sunxi_sdhci.h"

#define USART_DBG usart5_dbg

//#define CONFIG_BOOT_SPINAND
#define CONFIG_BOOT_SDCARD
#define CONFIG_BOOT_MMC

//#define CONFIG_ENABLE_CONSOLE
#define CONFIG_CONSOLE_USART usart5_dbg

#define CONFIG_FATFS_CACHE_SIZE		 (CONFIG_DTB_LOAD_ADDR - SDRAM_BASE) // in bytes
#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024 // (unit: 512B sectors)

#define CONFIG_CPU_FREQ 1200000000

#define CONFIG_ENABLE_CPU_FREQ_DUMP

#define CONFIG_KERNEL_FILENAME "zImage"
#define CONFIG_DTB_FILENAME	   "sun8i-t113-mangopi-dual.dtb"

#define CONFIG_CMD_LINE_ARGS                                                                                     \
	"mem=64M cma=8M root=/dev/mmcblk0p2 init=/sbin/init console=ttyS5,115200 earlyprintk=sunxi-uart,0x02501400 " \
	"rootwait"

#define CONFIG_KERNEL_LOAD_ADDR (SDRAM_BASE + (72 * 1024 * 1024))
#define CONFIG_DTB_LOAD_ADDR	(SDRAM_BASE + (64 * 1024 * 1024))

// 128KB erase sectors, so place them starting from 2nd sector
#define CONFIG_SPINAND_DTB_ADDR	   (128 * 2048)
#define CONFIG_SPINAND_KERNEL_ADDR (256 * 2048)

extern sunxi_usart_t USART_DBG;
extern sunxi_spi_t	 sunxi_spi0;

extern void board_init(void);
extern int	board_sdhci_init(void);

#endif
