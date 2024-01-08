#ifndef __SUNXI_CCU_H__
#define __SUNXI_CCU_H__

#define CCU_DMA_BGR_REG 0x070c

#define CCU_SMHC0_CLK_REG 0x0830 /* SMHC0 Clock Register */
#define CCU_SMHC1_CLK_REG 0x0834 /* SMHC1 Clock Register */
#define CCU_SMHC2_CLK_REG 0x0838 /* SMHC2 Clock Register */
#define CCU_SMHC_BGR_REG  0x084C /* SMHC Bus Gating Reset Register */

#define CCU_UART_BGR_REG 0x090C /* UART Bus Gating Reset Register */
#define CCU_TWI_BGR_REG	 0x091C /* TWI Bus Gating Reset Register */

#define CCU_SPI0_CLK_REG 0x0940 /* SPI0 Clock Register */
#define CCU_SPI1_CLK_REG 0x0944 /* SPI1 Clock Register */
#define CCU_SPI2_CLK_REG 0x0948 /* SPI2 Clock Register */
#define CCU_SPI3_CLK_REG 0x094C /* SPI3 Clock Register */
#define CCU_SPI_BGR_REG	 0x096C /* SPI Bus Gating Reset Register */

#define CCU_MBUS_MAT_CLK_GATING_REG (0x804)

/* MMC clock bit field */
#define CCU_MMC_CTRL_M(x)		  ((x)-1)
#define CCU_MMC_CTRL_N(x)		  ((x) << 8)
#define CCU_MMC_CTRL_OSCM24		  (0x0 << 24)
#define CCU_MMC_CTRL_PERI_400M	  (0x1 << 24)
#define CCU_MMC_CTRL_PERI_300M	  (0x2 << 24)
#define CCU_MMC_CTRL_PLL_PERIPH1X CCU_MMC_CTRL_PERI_400M
#define CCU_MMC_CTRL_PLL_PERIPH2X CCU_MMC_CTRL_PERI_300M
#define CCU_MMC_CTRL_ENABLE		  (0x1 << 31)
/* if doesn't have these delays */
#define CCU_MMC_CTRL_OCLK_DLY(a) ((void)(a), 0)
#define CCU_MMC_CTRL_SCLK_DLY(a) ((void)(a), 0)

#define CCU_MMC_BGR_SMHC0_GATE (1 << 0)
#define CCU_MMC_BGR_SMHC1_GATE (1 << 1)
#define CCU_MMC_BGR_SMHC2_GATE (1 << 2)

#define CCU_MMC_BGR_SMHC0_RST (1 << 16)
#define CCU_MMC_BGR_SMHC1_RST (1 << 17)
#define CCU_MMC_BGR_SMHC2_RST (1 << 18)

#endif
