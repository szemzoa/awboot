/*
 * driver/sdhci-t113.c
 *
 * Copyright(c) 2007-2022 Jianjun Jiang <8192542@qq.com>
 * Official site: http://xboot.org
 * Mobile phone: +86-18665388956
 * QQ: 8192542
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "main.h"
#include "sdmmc.h"
#include "debug.h"
#include "barrier.h"
#include "sunxi_sdhci.h"
#include "sunxi_gpio.h"
#include "sunxi_clk.h"

#define FALSE 0
#define TRUE  1

/*
 * Global control register bits
 */
#define SDXC_SOFT_RESET			  (1 << 0)
#define SDXC_FIFO_RESET			  (1 << 1)
#define SDXC_DMA_RESET			  (1 << 2)
#define SDXC_INTERRUPT_ENABLE_BIT (1 << 4)
#define SDXC_DMA_ENABLE_BIT		  (1 << 5)
#define SDXC_DEBOUNCE_ENABLE_BIT  (1 << 8)
#define SDXC_POSEDGE_LATCH_DATA	  (1 << 9)
#define SDXC_DDR_MODE			  (1 << 10)
#define SDXC_MEMORY_ACCESS_DONE	  (1 << 29)
#define SDXC_ACCESS_DONE_DIRECT	  (1 << 30)
#define SDXC_ACCESS_BY_AHB		  (1 << 31)
#define SDXC_ACCESS_BY_DMA		  (0 << 31)
#define SDXC_HARDWARE_RESET		  (SDXC_SOFT_RESET | SDXC_FIFO_RESET | SDXC_DMA_RESET)

/*
 * Clock control bits
 */
#define SDXC_CARD_CLOCK_ON (1 << 16)
#define SDXC_LOW_POWER_ON  (1 << 17)

/*
 * Bus width
 */
#define SDXC_WIDTH1 (0)
#define SDXC_WIDTH4 (1)

/*
 * Smc command bits
 */
#define SDXC_RESP_EXPIRE		(1 << 6)
#define SDXC_LONG_RESPONSE		(1 << 7)
#define SDXC_CHECK_RESPONSE_CRC (1 << 8)
#define SDXC_DATA_EXPIRE		(1 << 9)
#define SDXC_WRITE				(1 << 10)
#define SDXC_SEQUENCE_MODE		(1 << 11)
#define SDXC_SEND_AUTO_STOP		(1 << 12)
#define SDXC_WAIT_PRE_OVER		(1 << 13)
#define SDXC_STOP_ABORT_CMD		(1 << 14)
#define SDXC_SEND_INIT_SEQUENCE (1 << 15)
#define SDXC_UPCLK_ONLY			(1 << 21)
#define SDXC_READ_CEATA_DEV		(1 << 22)
#define SDXC_CCS_EXPIRE			(1 << 23)
#define SDXC_ENABLE_BIT_BOOT	(1 << 24)
#define SDXC_ALT_BOOT_OPTIONS	(1 << 25)
#define SDXC_BOOT_ACK_EXPIRE	(1 << 26)
#define SDXC_BOOT_ABORT			(1 << 27)
#define SDXC_VOLTAGE_SWITCH		(1 << 28)
#define SDXC_USE_HOLD_REGISTER	(1 << 29)
#define SDXC_START				(1 << 31)

/*
 * Interrupt bits
 */
#define SDXC_RESP_ERROR			 (1 << 1)
#define SDXC_COMMAND_DONE		 (1 << 2)
#define SDXC_DATA_OVER			 (1 << 3)
#define SDXC_TX_DATA_REQUEST	 (1 << 4)
#define SDXC_RX_DATA_REQUEST	 (1 << 5)
#define SDXC_RESP_CRC_ERROR		 (1 << 6)
#define SDXC_DATA_CRC_ERROR		 (1 << 7)
#define SDXC_RESP_TIMEOUT		 (1 << 8)
#define SDXC_DATA_TIMEOUT		 (1 << 9)
#define SDXC_VOLTAGE_CHANGE_DONE (1 << 10)
#define SDXC_FIFO_RUN_ERROR		 (1 << 11)
#define SDXC_HARD_WARE_LOCKED	 (1 << 12)
#define SDXC_START_BIT_ERROR	 (1 << 13)
#define SDXC_AUTO_COMMAND_DONE	 (1 << 14)
#define SDXC_END_BIT_ERROR		 (1 << 15)
#define SDXC_SDIO_INTERRUPT		 (1 << 16)
#define SDXC_CARD_INSERT		 (1 << 30)
#define SDXC_CARD_REMOVE		 (1 << 31)
#define SDXC_INTERRUPT_ERROR_BIT                                                                           \
	(SDXC_RESP_ERROR | SDXC_RESP_CRC_ERROR | SDXC_DATA_CRC_ERROR | SDXC_RESP_TIMEOUT | SDXC_DATA_TIMEOUT | \
	 SDXC_FIFO_RUN_ERROR | SDXC_HARD_WARE_LOCKED | SDXC_START_BIT_ERROR | SDXC_END_BIT_ERROR)
#define SDXC_INTERRUPT_DONE_BIT (SDXC_AUTO_COMMAND_DONE | SDXC_DATA_OVER | SDXC_COMMAND_DONE | SDXC_VOLTAGE_CHANGE_DONE)

#define SUNXI_MMC_GCTRL_SOFT_RESET	  (0x1 << 0)
#define SUNXI_MMC_GCTRL_FIFO_RESET	  (0x1 << 1)
#define SUNXI_MMC_GCTRL_DMA_RESET	  (0x1 << 2)
#define SUNXI_MMC_GCTRL_RESET		  (SUNXI_MMC_GCTRL_SOFT_RESET | SUNXI_MMC_GCTRL_FIFO_RESET | SUNXI_MMC_GCTRL_DMA_RESET)
#define SUNXI_MMC_GCTRL_DMA_ENABLE	  (0x1 << 5)
#define SUNXI_MMC_GCTRL_ACCESS_BY_AHB (0x1 << 31)

#define SUNXI_MMC_RINT_RESP_ERROR		   (0x1 << 1)
#define SUNXI_MMC_RINT_COMMAND_DONE		   (0x1 << 2)
#define SUNXI_MMC_RINT_DATA_OVER		   (0x1 << 3)
#define SUNXI_MMC_RINT_TX_DATA_REQUEST	   (0x1 << 4)
#define SUNXI_MMC_RINT_RX_DATA_REQUEST	   (0x1 << 5)
#define SUNXI_MMC_RINT_RESP_CRC_ERROR	   (0x1 << 6)
#define SUNXI_MMC_RINT_DATA_CRC_ERROR	   (0x1 << 7)
#define SUNXI_MMC_RINT_RESP_TIMEOUT		   (0x1 << 8)
#define SUNXI_MMC_RINT_DATA_TIMEOUT		   (0x1 << 9)
#define SUNXI_MMC_RINT_VOLTAGE_CHANGE_DONE (0x1 << 10)
#define SUNXI_MMC_RINT_FIFO_RUN_ERROR	   (0x1 << 11)
#define SUNXI_MMC_RINT_HARD_WARE_LOCKED	   (0x1 << 12)
#define SUNXI_MMC_RINT_START_BIT_ERROR	   (0x1 << 13)
#define SUNXI_MMC_RINT_AUTO_COMMAND_DONE   (0x1 << 14)
#define SUNXI_MMC_RINT_END_BIT_ERROR	   (0x1 << 15)
#define SUNXI_MMC_RINT_SDIO_INTERRUPT	   (0x1 << 16)
#define SUNXI_MMC_RINT_CARD_INSERT		   (0x1 << 30)
#define SUNXI_MMC_RINT_CARD_REMOVE		   (0x1 << 31)
#define SUNXI_MMC_RINT_INTERRUPT_ERROR_BIT                                                              \
	(SUNXI_MMC_RINT_RESP_ERROR | SUNXI_MMC_RINT_RESP_CRC_ERROR | SUNXI_MMC_RINT_DATA_CRC_ERROR |        \
	 SUNXI_MMC_RINT_RESP_TIMEOUT | SUNXI_MMC_RINT_DATA_TIMEOUT | SUNXI_MMC_RINT_VOLTAGE_CHANGE_DONE |   \
	 SUNXI_MMC_RINT_FIFO_RUN_ERROR | SUNXI_MMC_RINT_HARD_WARE_LOCKED | SUNXI_MMC_RINT_START_BIT_ERROR | \
	 SUNXI_MMC_RINT_END_BIT_ERROR) /* 0xbfc2 */
#define SUNXI_MMC_RINT_INTERRUPT_DONE_BIT                                                        \
	(SUNXI_MMC_RINT_AUTO_COMMAND_DONE | SUNXI_MMC_RINT_DATA_OVER | SUNXI_MMC_RINT_COMMAND_DONE | \
	 SUNXI_MMC_RINT_VOLTAGE_CHANGE_DONE)

/*
 * Status
 */
#define SDXC_RXWL_FLAG		(1 << 0)
#define SDXC_TXWL_FLAG		(1 << 1)
#define SDXC_FIFO_EMPTY		(1 << 2)
#define SDXC_FIFO_FULL		(1 << 3)
#define SDXC_CARD_PRESENT	(1 << 8)
#define SDXC_CARD_DATA_BUSY (1 << 9)
#define SDXC_DATA_FSM_BUSY	(1 << 10)
#define SDXC_DMA_REQUEST	(1 << 31)
#define SDXC_FIFO_SIZE		(16)

/*
 * Function select
 */
#define SDXC_CEATA_ON			  (0xceaa << 16)
#define SDXC_SEND_IRQ_RESPONSE	  (1 << 0)
#define SDXC_SDIO_READ_WAIT		  (1 << 1)
#define SDXC_ABORT_READ_DATA	  (1 << 2)
#define SDXC_SEND_CCSD			  (1 << 8)
#define SDXC_SEND_AUTO_STOPCCSD	  (1 << 9)
#define SDXC_CEATA_DEV_IRQ_ENABLE (1 << 10)

/* IDMA controller bus mod bit field */
#define SDXC_IDMAC_SOFT_RESET  BIT(0)
#define SDXC_IDMAC_FIX_BURST   BIT(1)
#define SDXC_IDMAC_IDMA_ON	   BIT(7)
#define SDXC_IDMAC_REFETCH_DES BIT(31)

/* IDMA status bit field */
#define SDXC_IDMAC_TRANSMIT_INTERRUPT	  BIT(0)
#define SDXC_IDMAC_RECEIVE_INTERRUPT	  BIT(1)
#define SDXC_IDMAC_FATAL_BUS_ERROR		  BIT(2)
#define SDXC_IDMAC_DESTINATION_INVALID	  BIT(4)
#define SDXC_IDMAC_CARD_ERROR_SUM		  BIT(5)
#define SDXC_IDMAC_NORMAL_INTERRUPT_SUM	  BIT(8)
#define SDXC_IDMAC_ABNORMAL_INTERRUPT_SUM BIT(9)
#define SDXC_IDMAC_HOST_ABORT_INTERRUPT	  BIT(10)
#define SDXC_IDMAC_IDLE					  (0 << 13)
#define SDXC_IDMAC_SUSPEND				  (1 << 13)
#define SDXC_IDMAC_DESC_READ			  (2 << 13)
#define SDXC_IDMAC_DESC_CHECK			  (3 << 13)
#define SDXC_IDMAC_READ_REQUEST_WAIT	  (4 << 13)
#define SDXC_IDMAC_WRITE_REQUEST_WAIT	  (5 << 13)
#define SDXC_IDMAC_READ					  (6 << 13)
#define SDXC_IDMAC_WRITE				  (7 << 13)
#define SDXC_IDMAC_DESC_CLOSE			  (8 << 13)

/*
 * If the idma-des-size-bits of property is ie 13, bufsize bits are:
 *  Bits  0-12: buf1 size
 *  Bits 13-25: buf2 size
 *  Bits 26-31: not used
 * Since we only ever set buf1 size, we can simply store it directly.
 */
#define SDXC_IDMAC_DES0_DIC BIT(1) /* disable interrupt on completion */
#define SDXC_IDMAC_DES0_LD	BIT(2) /* last descriptor */
#define SDXC_IDMAC_DES0_FD	BIT(3) /* first descriptor */
#define SDXC_IDMAC_DES0_CH	BIT(4) /* chain mode */
#define SDXC_IDMAC_DES0_ER	BIT(5) /* end of ring */
#define SDXC_IDMAC_DES0_CES BIT(30) /* card error summary */
#define SDXC_IDMAC_DES0_OWN BIT(31) /* 1-idma owns it, 0-host owns it */

#define CONFIG_SYS_CACHELINE_SIZE 16
#define DTO_MAX					  200
#define SUNXI_DMA_TL_TM5_V5P3X    ((0x3<<28)|(15<<16)|240)
#define SUNXI_MMC_STATUS_FIFO_LEVEL(x)	(((x) >> 17) & 0x3fff)

static void set_read_timeout(sdhci_t *sdhci, u32 timeout)
{
	u32 rval	 = 0;
	u32 rdto_clk = 0;
	u32 mode_2x	 = 0;

	rdto_clk = sdhci->clock / 1000 * timeout;
	rval	 = sdhci->reg->ntsr;
	mode_2x	 = rval & (0x1 << 31);

	if ((sdhci->width == MMC_BUS_WIDTH_4_DDR && mode_2x)) {
		rdto_clk = rdto_clk << 1;
	}

	rval = sdhci->reg->gctrl;
	/*ddr50 mode don't use 256x timeout unit*/
	if (rdto_clk > 0xffffff && sdhci->width == MMC_BUS_WIDTH_4_DDR) {
		rdto_clk = (rdto_clk + 255) / 256;
		rval |= (0x1 << 11);
	} else {
		rdto_clk = 0xffffff;
		rval &= ~(0x1 << 11);
	}
	sdhci->reg->gctrl = rval;

	rval = sdhci->reg->timeout;
	rval &= ~(0xffffff << 8);
	rval |= (rdto_clk << 8);
	sdhci->reg->timeout = rval;

	trace("rdtoclk:%u, reg-tmout:%u, gctl:%x, clock:%u, nstr:%x\n", rdto_clk, sdhci->reg->timeout, sdhci->reg->gctrl,
		  sdhci->clock, sdhci->reg->ntsr);
}

static int prepare_dma(sdhci_t *sdhci, sdhci_data_t *data)
{
	sdhci_idma_desc_t *pdes		= sdhci->dma_desc;
	u32				   byte_cnt = data->blksz * data->blkcnt;
	u8				*buff;
	u32				   des_idx		 = 0;
	u32				   buff_frag_num = 0;
	u32				   remain;
	u32				   i;

	buff		  = data->buf;
	buff_frag_num = byte_cnt >> SDXC_DES_NUM_SHIFT;
	remain		  = byte_cnt & (SDXC_DES_BUFFER_MAX_LEN - 1);
	if (remain)
		buff_frag_num++;
	else
		remain = SDXC_DES_BUFFER_MAX_LEN - 1;

	for (i = 0; i < buff_frag_num; i++, des_idx++) {
		memset((void *)&pdes[des_idx], 0, sizeof(sdhci_idma_desc_t));
		pdes[des_idx].des_chain = 1;
		pdes[des_idx].own		= 1;
		pdes[des_idx].dic		= 1;
		if (buff_frag_num > 1 && i != buff_frag_num - 1)
			pdes[des_idx].data_buf_sz = SDXC_DES_BUFFER_MAX_LEN - 1;
		else
			pdes[des_idx].data_buf_sz = remain;
		pdes[des_idx].buf_addr = ((u32)buff + i * SDXC_DES_BUFFER_MAX_LEN) >> 2;
		if (i == 0)
			pdes[des_idx].first_desc = 1;

		if (i == buff_frag_num - 1) {
			pdes[des_idx].dic	   = 0;
			pdes[des_idx].last_desc = 1;
			pdes[des_idx].next_desc_addr = 0;
		} else {
			pdes[des_idx].next_desc_addr = ((u32)&pdes[des_idx + 1]) >> 2;
		}
		trace("SMHC: frag %d, remain %d, des[%d] = 0x%08x:\r\n"
			  "  [0] = 0x%08x, [1] = 0x%08x, [2] = 0x%08x, [3] = 0x%08x\r\n",
			  i, remain, des_idx, (u32)(&pdes[des_idx]), (u32)((u32 *)&pdes[des_idx])[0],
			  (u32)((u32 *)&pdes[des_idx])[1], (u32)((u32 *)&pdes[des_idx])[2], (u32)((u32 *)&pdes[des_idx])[3]);
	}

	wmb();

	/*
	 * GCTRLREG
	 * GCTRL[2]	: DMA reset
	 * GCTRL[5]	: DMA enable
	 *
	 * IDMACREG
	 * IDMAC[0]	: IDMA soft reset
	 * IDMAC[1]	: IDMA fix burst flag
	 * IDMAC[7]	: IDMA on
	 *
	 * IDIECREG
	 * IDIE[0]	: IDMA transmit interrupt flag
	 * IDIE[1]	: IDMA receive interrupt flag
	 */
	sdhci->reg->idst = 0x337; // clear interrupt status
	sdhci->reg->gctrl |= SDXC_DMA_ENABLE_BIT | SDXC_DMA_RESET; /* dma enable */
	sdhci->reg->dmac = SDXC_IDMAC_SOFT_RESET; /* idma reset */
	while (sdhci->reg->dmac & SDXC_IDMAC_SOFT_RESET) {
	} /* wait idma reset done */

	sdhci->reg->dmac = SDXC_IDMAC_FIX_BURST | SDXC_IDMAC_IDMA_ON; /* idma on */
	sdhci->reg->idie &= ~(SDXC_IDMAC_TRANSMIT_INTERRUPT | SDXC_IDMAC_RECEIVE_INTERRUPT);
	if (data->flag & MMC_DATA_WRITE)
		sdhci->reg->idie |= SDXC_IDMAC_TRANSMIT_INTERRUPT;
	else
		sdhci->reg->idie |= SDXC_IDMAC_RECEIVE_INTERRUPT;

	sdhci->reg->dlba	  = (u32)pdes  >> 2;
	sdhci->reg->ftrglevel = sdhci->dma_trglvl;

	return 0;
}

static int wait_done(sdhci_t *sdhci, sdhci_data_t *dat, u32 timeout_msecs, u32 flag, bool dma)
{
	u32 status;
	u32 done  = 0;
	u32 start = time_ms();
	trace("SMHC: wait for flag 0x%x\r\n", flag);
	do {
		status = sdhci->reg->rint;
		if ((time_ms() > (start + timeout_msecs))) {
			warning("SMHC: wait timeout %x status %x flag %x\r\n", status & SUNXI_MMC_RINT_INTERRUPT_ERROR_BIT, status,
					flag);
			return -1;
		} else if ((status & SUNXI_MMC_RINT_INTERRUPT_ERROR_BIT)) {
			warning("SMHC: error 0x%x status 0x%x\r\n", status & SUNXI_MMC_RINT_INTERRUPT_ERROR_BIT, status & ~SUNXI_MMC_RINT_INTERRUPT_ERROR_BIT);
			return -1;
		}
		if (dat && dma && (dat->blkcnt * dat->blksz) > 0)
			done = ((status & flag) && (sdhci->reg->idst & SDXC_IDMAC_RECEIVE_INTERRUPT)) ? 1 : 0;
		else
			done = (status & flag);
	} while (!done);

	return 0;
}

static bool read_bytes(sdhci_t *sdhci, sdhci_data_t *dat)
{
	u32	 count = dat->blkcnt * dat->blksz;
	u32 *tmp   = (u32 *)dat->buf;
	u32	 status, err, done;
	u32	 timeout = time_ms() + count;
	u32  in_fifo;

	if (timeout < 250)
		timeout = 250;

	trace("SMHC: read %u\r\n", count);

	status = sdhci->reg->status;
	err	   = sdhci->reg->rint & SDXC_INTERRUPT_ERROR_BIT;
	if (err)
		warning("SMHC: interrupt error 0x%x status 0x%x\r\n", err & SDXC_INTERRUPT_ERROR_BIT, status);

	while ((!err) && (count >= sizeof(sdhci->reg->fifo))) {
		while (sdhci->reg->status & SDXC_FIFO_EMPTY) {
			if (time_ms() > timeout) {
				warning("SMHC: read timeout\r\n");
				return FALSE;
			}
		}
		in_fifo = SUNXI_MMC_STATUS_FIFO_LEVEL(status);
		count -= sizeof(sdhci->reg->fifo) * in_fifo;
		while (in_fifo--) {
			*(tmp++) = sdhci->reg->fifo;
		}

		status = sdhci->reg->status;
		err	   = sdhci->reg->rint & SDXC_INTERRUPT_ERROR_BIT;
	}

	do {
		status = sdhci->reg->rint;

		err = status & SDXC_INTERRUPT_ERROR_BIT;
		if (dat->blkcnt > 1)
			done = status & SDXC_AUTO_COMMAND_DONE;
		else
			done = status & SDXC_DATA_OVER;

	} while (!done && !err);

	if (err & SDXC_INTERRUPT_ERROR_BIT) {
		warning("SMHC: interrupt error 0x%x status 0x%x\r\n", err & SDXC_INTERRUPT_ERROR_BIT, status);
		return FALSE;
	}

	if (count) {
		warning("SMHC: read %u leftover\r\n", count);
		return FALSE;
	}
	return TRUE;
}

static bool write_bytes(sdhci_t *sdhci, sdhci_data_t *dat)
{
	uint64_t count = dat->blkcnt * dat->blksz;
	u32		*tmp   = (u32 *)dat->buf;
	u32		 status, err, done;
	u32		 timeout = time_ms() + count;

	if (timeout < 250)
		timeout = 250;

	trace("SMHC: write %u\r\n", count);

	status = sdhci->reg->status;
	err	   = sdhci->reg->rint & SDXC_INTERRUPT_ERROR_BIT;
	while (!err && count) {
		while (sdhci->reg->status & SDXC_FIFO_FULL) {
			if (time_ms() > timeout) {
				warning("SMHC: write timeout\r\n");
				return FALSE;
			}
		}
		sdhci->reg->fifo = *(tmp++);
		count -= sizeof(u32);

		status = sdhci->reg->status;
		err	   = sdhci->reg->rint & SDXC_INTERRUPT_ERROR_BIT;
	}

	do {
		status = sdhci->reg->rint;
		err	   = status & SDXC_INTERRUPT_ERROR_BIT;
		if (dat->blkcnt > 1)
			done = status & SDXC_AUTO_COMMAND_DONE;
		else
			done = status & SDXC_DATA_OVER;
	} while (!done && !err);

	if (err & SDXC_INTERRUPT_ERROR_BIT)
		return FALSE;
	// sdhci->reg->gctrl |= SDXC_FIFO_RESET;

	if (count)
		return FALSE;
	return TRUE;
}

static bool transfer_command(sdhci_t *sdhci, sdhci_cmd_t *cmd, sdhci_data_t *dat)
{
	u32	 cmdval = 0;
	u32	 status = 0;
	u32	 timeout;
	bool dma = false;

	trace("SMHC: CMD%u 0x%x dlen:%u\r\n", cmd->idx, cmd->arg, dat ? dat->blkcnt * dat->blksz : 0);

	if (cmd->idx == MMC_STOP_TRANSMISSION) {
		timeout = time_ms();
		do {
			status = sdhci->reg->status;
			if (time_ms() - timeout > 10) {
				sdhci->reg->gctrl = SDXC_HARDWARE_RESET;
				sdhci->reg->rint  = 0xffffffff;
				warning("SMHC: stop timeout\r\n");
				return FALSE;
			}
		} while (status & SDXC_CARD_DATA_BUSY);
		return TRUE;
	}

	if (cmd->resptype & MMC_RSP_PRESENT) {
		cmdval |= SDXC_RESP_EXPIRE;
		if (cmd->resptype & MMC_RSP_136)
			cmdval |= SDXC_LONG_RESPONSE;
		if (cmd->resptype & MMC_RSP_CRC)
			cmdval |= SDXC_CHECK_RESPONSE_CRC;
	}

	if (dat) {
		cmdval |= SDXC_DATA_EXPIRE | SDXC_WAIT_PRE_OVER;
		set_read_timeout(sdhci, DTO_MAX);

		if (dat->flag & MMC_DATA_WRITE) {
			cmdval |= SDXC_WRITE;
			sdhci->reg->idst |= SDXC_IDMAC_TRANSMIT_INTERRUPT; // clear TX status
		}
		if (dat->flag & MMC_DATA_READ)
			sdhci->reg->idst |= SDXC_IDMAC_RECEIVE_INTERRUPT; // clear RX status
	}

	if (cmd->idx == MMC_WRITE_MULTIPLE_BLOCK || cmd->idx == MMC_READ_MULTIPLE_BLOCK)
		cmdval |= SDXC_SEND_AUTO_STOP;

	sdhci->reg->rint = 0xffffffff; // Clear status
	sdhci->reg->arg	 = cmd->arg;

	if (dat && (dat->blkcnt * dat->blksz) > 64) {
		dma = true;
		sdhci->reg->gctrl &= ~SUNXI_MMC_GCTRL_ACCESS_BY_AHB;
		prepare_dma(sdhci, dat);
		sdhci->reg->cmd = cmdval | cmd->idx | SDXC_START; // Start
	} else if (dat && (dat->blkcnt * dat->blksz) > 0) {
		sdhci->reg->gctrl |= SUNXI_MMC_GCTRL_ACCESS_BY_AHB;
		sdhci->reg->cmd = cmdval | cmd->idx | SDXC_START; // Start
		if (dat->flag & MMC_DATA_READ && !read_bytes(sdhci, dat))
			return FALSE;
		else if (dat->flag & MMC_DATA_WRITE && !write_bytes(sdhci, dat))
			return FALSE;
	} else {
		sdhci->reg->gctrl |= SUNXI_MMC_GCTRL_ACCESS_BY_AHB;
		sdhci->reg->cmd = cmdval | cmd->idx | SDXC_START; // Start
	}

	if (wait_done(sdhci, 0, 100, SUNXI_MMC_RINT_COMMAND_DONE, false)) {
		warning("SMHC: cmd timeout\r\n");
		return FALSE;
	}

	if (dat && wait_done(sdhci, dat, 6000,
						 dat->blkcnt > 1 ? SUNXI_MMC_RINT_AUTO_COMMAND_DONE : SUNXI_MMC_RINT_DATA_OVER, dma)) {
		warning("SMHC: data timeout\r\n");
		return FALSE;
	}

	if (cmd->resptype & MMC_RSP_BUSY) {
		timeout = time_ms();
		do {
			status = sdhci->reg->status;
			if (time_ms() - timeout > 10) {
				sdhci->reg->gctrl = SDXC_HARDWARE_RESET;
				sdhci->reg->rint  = 0xffffffff;
				warning("SMHC: busy timeout\r\n");
				return FALSE;
			}
		} while (status & SDXC_CARD_DATA_BUSY);
	}

	if (cmd->resptype & MMC_RSP_136) {
		cmd->response[0] = sdhci->reg->resp3;
		cmd->response[1] = sdhci->reg->resp2;
		cmd->response[2] = sdhci->reg->resp1;
		cmd->response[3] = sdhci->reg->resp0;
	} else {
		cmd->response[0] = sdhci->reg->resp0;
	}

	// Cleanup and disable IDMA
	if (dat && dma) {
		status			 = sdhci->reg->idst;
		sdhci->reg->idst = status;
		sdhci->reg->idie = 0;
		sdhci->reg->dmac = 0;
		sdhci->reg->gctrl &= ~SUNXI_MMC_GCTRL_DMA_ENABLE;
	}

	return TRUE;
}

static bool transfer_data(sdhci_t *sdhci, sdhci_cmd_t *cmd, sdhci_data_t *dat)
{
	u32 dlen = (u32)(dat->blkcnt * dat->blksz);

	sdhci->reg->blksz	= dat->blksz;
	sdhci->reg->bytecnt = dlen;

	return transfer_command(sdhci, cmd, dat);
}

bool sdhci_reset(sdhci_t *sdhci)
{
	sdhci->reg->gctrl = SDXC_HARDWARE_RESET;
	return TRUE;
}

bool sdhci_set_width(sdhci_t *sdhci, u32 width)
{
	const char *mode = "1 bit";
	switch (width) {
		case MMC_BUS_WIDTH_1:
			sdhci->reg->width = SDXC_WIDTH1;
			sdhci->reg->gctrl &= ~SDXC_DDR_MODE;
			break;
		case MMC_BUS_WIDTH_4:
			sdhci->reg->width = SDXC_WIDTH4;
			sdhci->reg->gctrl &= ~SDXC_DDR_MODE;
			mode = "4 bit";
			break;
		case MMC_BUS_WIDTH_4_DDR:
			sdhci->reg->width = SDXC_WIDTH4;
			sdhci->reg->gctrl |= SDXC_DDR_MODE;
			mode = "4 bit DDR";
			break;
		default:
			error("SMHC: %u width value invalid\r\n", width);
			return FALSE;
	}
	trace("SMHC: set width to %s\r\n", mode);
	return TRUE;
}

static bool update_clk(sdhci_t *sdhci)
{
	u32 cmd = (1U << 31) | (1 << 21) | (1 << 13);

	sdhci->reg->cmd = cmd;
	u32 timeout		= time_ms();
	do {
		if (time_ms() - timeout > 10)
			return FALSE;
	} while (sdhci->reg->cmd & 0x80000000);
	sdhci->reg->rint = sdhci->reg->rint;
	return TRUE;
}

bool sdhci_set_clock(sdhci_t *sdhci, u32 clock)
{
	u32 ratio = (sdhci->pclk / clock);

	if (clock <= 1000000) {
		trace("SMHC: set clock to %.2fKHz\r\n", (f32)((f32)clock / 1000.0));
	} else {
		trace("SMHC: set clock to %.2fMHz\r\n", (f32)((f32)clock / 1000000.0));
	}
	trace("SMHC: clock ratio %u\r\n", ratio);

	if ((ratio & 0xff) != ratio)
		return FALSE;
	sdhci->reg->clkcr &= ~(1 << 16);
	sdhci->reg->clkcr = ratio & 0xff;
	if (!update_clk(sdhci))
		return FALSE;
	sdhci->reg->clkcr |= (3 << 16);
	if (!update_clk(sdhci))
		return FALSE;
	return TRUE;
}

bool sdhci_transfer(sdhci_t *sdhci, sdhci_cmd_t *cmd, sdhci_data_t *dat)
{
	if (!dat)
		return transfer_command(sdhci, cmd, dat);

	return transfer_data(sdhci, cmd, dat);
}

int sunxi_sdhci_init(sdhci_t *sdhci)
{
	u32 addr, val;

	sunxi_gpio_init(sdhci->gpio_clk.pin, sdhci->gpio_clk.mux);
	sunxi_gpio_set_pull(sdhci->gpio_clk.pin, GPIO_PULL_UP);

	sunxi_gpio_init(sdhci->gpio_cmd.pin, sdhci->gpio_cmd.mux);
	sunxi_gpio_set_pull(sdhci->gpio_cmd.pin, GPIO_PULL_UP);

	sunxi_gpio_init(sdhci->gpio_d0.pin, sdhci->gpio_d0.mux);
	sunxi_gpio_set_pull(sdhci->gpio_d0.pin, GPIO_PULL_UP);

	sunxi_gpio_init(sdhci->gpio_d1.pin, sdhci->gpio_d1.mux);
	sunxi_gpio_set_pull(sdhci->gpio_d1.pin, GPIO_PULL_UP);

	sunxi_gpio_init(sdhci->gpio_d2.pin, sdhci->gpio_d2.mux);
	sunxi_gpio_set_pull(sdhci->gpio_d2.pin, GPIO_PULL_UP);

	sunxi_gpio_init(sdhci->gpio_d3.pin, sdhci->gpio_d3.mux);
	sunxi_gpio_set_pull(sdhci->gpio_d3.pin, GPIO_PULL_UP);

	/* De-assert reset */
	addr = T113_CCU_BASE + CCU_SMHC_BGR_REG;
	val	 = read32(addr);
	val |= (1 << 16);
	write32(addr, val);

	/* Open the clock gate for sdhci0 */
	addr = T113_CCU_BASE + CCU_SMHC0_CLK_REG;
	val	 = read32(addr);
	val |= (1 << 31) | (1 << 24) | 0 << 8 | 5; /* 600/6=100MHz */
	write32(addr, val);

	sdhci->pclk = sunxi_clk_get_peri1x_rate() / 6;

	/* sdhc0 clock gate pass */
	addr = T113_CCU_BASE + CCU_SMHC_BGR_REG;
	val	 = read32(addr);
	val |= (1 << 0);
	write32(addr, val);

	udelay(10);

	sdhci->reg->gctrl = SDXC_HARDWARE_RESET;
	sdhci->reg->rint  = 0xffffffff;

	sdhci->dma_trglvl = SUNXI_DMA_TL_TM5_V5P3X;

	udelay(100);

	return 0;
}
