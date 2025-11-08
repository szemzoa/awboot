#ifndef __BOARD_H__
#define __BOARD_H__

#include "dram.h"
#include "sunxi_spi.h"
#include "sunxi_usart.h"
#include "sunxi_sdhci.h"

#define CONFIG_KERNEL_FILENAME "zImage"
#define CONFIG_DTB_FILENAME	   "sun8i-t113-mangopi-dual.dtb"
#ifndef CONFIG_INITRD_FILENAME
#define CONFIG_INITRD_FILENAME ""
#endif
#ifndef CONFIG_MMC_ENABLE_RSTN
#define CONFIG_MMC_ENABLE_RSTN 0
#endif

#define RTC_BKP_REG(n) *((uint32_t *)((0x07090100) + (n * 4)))

#define MB(x) ((uint32_t)(x) * 1024U * 1024U)
#define CONFIG_INITRAMFS_MAX_SIZE   MB(25)

#define CONFIG_KERNEL_LOAD_ADDR	    (SDRAM_BASE + MB(32))
#define CONFIG_DTB_GUARD_SIZE	      MB(1)

#define CONFIG_INITRD_ALIGNMENT	  64U

// FEL mailbox layout (must match host FEL script)
#define CONFIG_FEL_MAILBOX_BASE    0x43100000U
#define CONFIG_MAIL_INITRD_SIZE_ADDR  (CONFIG_FEL_MAILBOX_BASE + 0x0U)
#define CONFIG_MAIL_INITRD_START_ADDR (CONFIG_FEL_MAILBOX_BASE + 0x4U)
#define CONFIG_MAIL_DTB_ADDR_ADDR      (CONFIG_FEL_MAILBOX_BASE + 0x8U)
#define CONFIG_MAIL_KERNEL_ADDR_ADDR   (CONFIG_FEL_MAILBOX_BASE + 0xCU)

#define CONFIG_CONF_FILENAME	"boot.cfg"
#define CONFIG_DEFAULT_BOOT_CMD "console=ttyS3,115200 earlycon"
#define CONFIG_BOOT_MAX_TRIES	2

/* Boot source configuration flags (1 = enabled) */
#define CONFIG_BOOT_SPINAND 0
#define CONFIG_BOOT_SDCARD	0
#define CONFIG_BOOT_MMC		1

#define CONFIG_FATFS_CACHE_SIZE		 36 // (unit: 512B sectors, multiples of 8 to match FAT's 4KB)
#define CONFIG_SDMMC_SPEED_TEST_SIZE 2048 // (unit: 512B sectors)

#define CONFIG_CPU_FREQ 1200000000

// #define CONFIG_ENABLE_CPU_FREQ_DUMP

// 128KB erase sectors, 2KB pages, so place them starting from 2nd sector
#define CONFIG_SPINAND_DTB_ADDR	   (128 * 2048)
#define CONFIG_SPINAND_KERNEL_ADDR (256 * 2048)

#define CONFIG_PSCI_DRAM_RESERVE 0x00010000U

#define LED_BOARD  1

extern sunxi_usart_t usart0_dbg;
extern sunxi_usart_t usart3_dbg;
extern sunxi_usart_t usart5_dbg;
extern sunxi_spi_t	 sunxi_spi0;

extern sdhci_t sdhci0;
extern sdhci_t sdhci2;

void board_init(void);
void board_set_led(uint8_t num, uint8_t on);

#define USART_DBG usart0_dbg
#define SDHCI sdhci0

#endif
