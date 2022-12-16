#ifndef __SDHCI_H__
#define __SDHCI_H__

#include "sunxi_gpio.h"

typedef enum {
	MMC_CLK_400K = 0,
	MMC_CLK_25M,
	MMC_CLK_50M,
	MMC_CLK_50M_DDR,
	MMC_CLK_100M,
	MMC_CLK_150M,
	MMC_CLK_200M
} smhc_clk_t;

typedef struct {
	volatile u32 gctrl; /* (0x00) SMC Global Control Register */
	volatile u32 clkcr; /* (0x04) SMC Clock Control Register */
	volatile u32 timeout; /* (0x08) SMC Time Out Register */
	volatile u32 width; /* (0x0C) SMC Bus Width Register */
	volatile u32 blksz; /* (0x10) SMC Block Size Register */
	volatile u32 bytecnt; /* (0x14) SMC Byte Count Register */
	volatile u32 cmd; /* (0x18) SMC Command Register */
	volatile u32 arg; /* (0x1C) SMC Argument Register */
	volatile u32 resp0; /* (0x20) SMC Response Register 0 */
	volatile u32 resp1; /* (0x24) SMC Response Register 1 */
	volatile u32 resp2; /* (0x28) SMC Response Register 2 */
	volatile u32 resp3; /* (0x2C) SMC Response Register 3 */
	volatile u32 imask; /* (0x30) SMC Interrupt Mask Register */
	volatile u32 mint; /* (0x34) SMC Masked Interrupt Status Register */
	volatile u32 rint; /* (0x38) SMC Raw Interrupt Status Register */
	volatile u32 status; /* (0x3C) SMC Status Register */
	volatile u32 ftrglevel; /* (0x40) SMC FIFO Threshold Watermark Register */
	volatile u32 funcsel; /* (0x44) SMC Function Select Register */
	volatile u32 cbcr; /* (0x48) SMC CIU Byte Count Register */
	volatile u32 bbcr; /* (0x4C) SMC BIU Byte Count Register */
	volatile u32 dbgc; /* (0x50) SMC Debug Enable Register */
	volatile u32 csdc; /* (0x54) CRC status detect control register*/
	volatile u32 a12a; /* (0x58)Auto command 12 argument*/
	volatile u32 ntsr; /* (0x5c)SMC2 Newtiming Set Register */
	volatile u32 res1[6]; /* (0x60~0x74) */
	volatile u32 hwrst; /* (0x78) SMC eMMC Hardware Reset Register */
	volatile u32 res2; /*  (0x7c) */
	volatile u32 dmac; /*  (0x80) SMC IDMAC Control Register */
	volatile u32 dlba; /*  (0x84) SMC IDMAC Descriptor List Base Address Register */
	volatile u32 idst; /*  (0x88) SMC IDMAC Status Register */
	volatile u32 idie; /*  (0x8C) SMC IDMAC Interrupt Enable Register */
	volatile u32 chda; /*  (0x90) */
	volatile u32 cbda; /*  (0x94) */
	volatile u32 res3[26]; /*  (0x98~0xff) */
	volatile u32 thldc; /*  (0x100) Card Threshold Control Register */
	volatile u32 sfc; /* (0x104) Sample Fifo Control Register */
	volatile u32 res4[1]; /*  (0x10b) */
	volatile u32 dsbd; /* (0x10c) eMMC4.5 DDR Start Bit Detection Control */
	volatile u32 res5[12]; /* (0x110~0x13c) */
	volatile u32 drv_dl; /* (0x140) Drive Delay Control register*/
	volatile u32 samp_dl; /* (0x144) Sample Delay Control register*/
	volatile u32 ds_dl; /* (0x148) Data Strobe Delay Control Register */
	volatile u32 ntdc; /* (0x14C) HS400 New Timing Delay Control Register */
	volatile u32 res6[4]; /* (0x150~0x15f) */
	volatile u32 skew_dat0_dl; /*(0x160) deskew data0 delay control register*/
	volatile u32 skew_dat1_dl; /*(0x164) deskew data1 delay control register*/
	volatile u32 skew_dat2_dl; /*(0x168) deskew data2 delay control register*/
	volatile u32 skew_dat3_dl; /*(0x16c) deskew data3 delay control register*/
	volatile u32 skew_dat4_dl; /*(0x170) deskew data4 delay control register*/
	volatile u32 skew_dat5_dl; /*(0x174) deskew data5 delay control register*/
	volatile u32 skew_dat6_dl; /*(0x178) deskew data6 delay control register*/
	volatile u32 skew_dat7_dl; /*(0x17c) deskew data7 delay control register*/
	volatile u32 skew_ds_dl; /*(0x180) deskew ds delay control register*/
	volatile u32 skew_ctrl; /*(0x184) deskew control control register*/
	volatile u32 res8[30]; /* (0x188~0x1ff) */
	volatile u32 fifo; /* (0x200) SMC FIFO Access Address */
	volatile u32 res7[63]; /* (0x201~0x2FF)*/
	volatile u32 vers; /* (0x300) SMHC Version Register */
} sdhci_reg_t;

typedef struct {
	u32 idx;
	u32 arg;
	u32 resptype;
	u32 response[4];
} sdhci_cmd_t;

typedef struct {
	u8 *buf;
	u32 flag;
	u32 blksz;
	u32 blkcnt;
} sdhci_data_t;

#define SMHC_DES_NUM_SHIFT		12 /* smhc2!! */
#define SMHC_DES_BUFFER_MAX_LEN (1 << SMHC_DES_NUM_SHIFT)
typedef struct {
	u32 : 1, dic : 1, /* disable interrupt on completion */
		last_desc : 1, /* 1-this data buffer is the last buffer */
		first_desc : 1, /* 1-data buffer is the first buffer, 0-data buffer contained in the next descriptor is 1st
						  buffer */
		des_chain : 1, /* 1-the 2nd address in the descriptor is the next descriptor address */
		// end_of_ring : 1, /* 1-last descriptor flag when using dual data buffer in descriptor */
		: 25, err_flag : 1, /* transfer error flag */
		own : 1; /* des owner:1-idma owns it, 0-host owns it */

	u32 data_buf_sz : SMHC_DES_NUM_SHIFT, data_buf_dummy : (32 - SMHC_DES_NUM_SHIFT);

	u32 buf_addr;
	u32 next_desc_addr;

} sdhci_idma_desc_t __attribute__((aligned(8)));

typedef struct {
	char		 *name;
	sdhci_reg_t *reg;
	u32			 reset;

	u32		   voltage;
	u32		   width;
	smhc_clk_t clock;
	u32		   pclk;
	u8		   odly[6];
	u8		   sdly[6];

	sdhci_idma_desc_t dma_desc[32];
	u32				  dma_trglvl;

	bool removable;
	bool isspi;

	gpio_mux_t gpio_d0;
	gpio_mux_t gpio_d1;
	gpio_mux_t gpio_d2;
	gpio_mux_t gpio_d3;
	gpio_mux_t gpio_cmd;
	gpio_mux_t gpio_clk;

} sdhci_t;

extern sdhci_t sdhci0;

bool sdhci_reset(sdhci_t *hci);
bool sdhci_set_voltage(sdhci_t *hci, u32 voltage);
bool sdhci_set_width(sdhci_t *hci, u32 width);
bool sdhci_set_clock(sdhci_t *hci, smhc_clk_t hz);
bool sdhci_transfer(sdhci_t *hci, sdhci_cmd_t *cmd, sdhci_data_t *dat);
int	 sunxi_sdhci_init(sdhci_t *sdhci);

#endif /* __SDHCI_H__ */
