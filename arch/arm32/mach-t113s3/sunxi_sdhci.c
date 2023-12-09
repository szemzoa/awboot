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
#define SMHC_GCTRL_SOFT_RESET		  (1 << 0)
#define SMHC_GCTRL_FIFO_RESET		  (1 << 1)
#define SMHC_GCTRL_DMA_RESET		  (1 << 2)
#define SMHC_GCTRL_INTERRUPT_ENABLE	  (1 << 4)
#define SMHC_GCTRL_DMA_ENABLE		  (1 << 5)
#define SMHC_GCTRL_DEBOUNCE_ENABLE	  (1 << 8)
#define SMHC_GCTRL_POSEDGE_LATCH_DATA (1 << 9)
#define SMHC_GCTRL_DDR_MODE			  (1 << 10)
#define SMHC_GCTRL_MEMORY_ACCESS_DONE (1 << 29)
#define SMHC_GCTRL_ACCESS_DONE_DIRECT (1 << 30)
#define SMHC_GCTRL_ACCESS_BY_AHB	  (1 << 31)
#define SMHC_GCTRL_ACCESS_BY_DMA	  (0 << 31)
#define SMHC_GCTRL_HARDWARE_RESET	  (SMHC_GCTRL_SOFT_RESET | SMHC_GCTRL_FIFO_RESET | SMHC_GCTRL_DMA_RESET)

/*
 * Clock control bits
 */
#define SMHC_CLKCR_MASK_D0		 (1 << 31)
#define SMHC_CLKCR_CARD_CLOCK_ON (1 << 16)
#define SMHC_CLKCR_LOW_POWER_ON	 (1 << 17)
#define SMHC_CLKCR_CLOCK_DIV(n)	 ((n - 1) & 0xff)

/*
 * Bus width
 */
#define SMHC_WIDTH_1BIT (0)
#define SMHC_WIDTH_4BIT (1)

/*
 * Smc command bits
 */
#define SMHC_CMD_RESP_EXPIRE		(1 << 6)
#define SMHC_CMD_LONG_RESPONSE		(1 << 7)
#define SMHC_CMD_CHECK_RESPONSE_CRC (1 << 8)
#define SMHC_CMD_DATA_EXPIRE		(1 << 9)
#define SMHC_CMD_WRITE				(1 << 10)
#define SMHC_CMD_SEQUENCE_MODE		(1 << 11)
#define SMHC_CMD_SEND_AUTO_STOP		(1 << 12)
#define SMHC_CMD_WAIT_PRE_OVER		(1 << 13)
#define SMHC_CMD_STOP_ABORT_CMD		(1 << 14)
#define SMHC_CMD_SEND_INIT_SEQUENCE (1 << 15)
#define SMHC_CMD_UPCLK_ONLY			(1 << 21)
#define SMHC_CMD_READ_CEATA_DEV		(1 << 22)
#define SMHC_CMD_CCS_EXPIRE			(1 << 23)
#define SMHC_CMD_ENABLE_BIT_BOOT	(1 << 24)
#define SMHC_CMD_ALT_BOOT_OPTIONS	(1 << 25)
#define SMHC_CMD_BOOT_ACK_EXPIRE	(1 << 26)
#define SMHC_CMD_BOOT_ABORT			(1 << 27)
#define SMHC_CMD_VOLTAGE_SWITCH		(1 << 28)
#define SMHC_CMD_USE_HOLD_REGISTER	(1 << 29)
#define SMHC_CMD_START				(1 << 31)

/*
 * Interrupt bits
 */
#define SMHC_RINT_RESP_ERROR		  (0x1 << 1)
#define SMHC_RINT_COMMAND_DONE		  (0x1 << 2)
#define SMHC_RINT_DATA_OVER			  (0x1 << 3)
#define SMHC_RINT_TX_DATA_REQUEST	  (0x1 << 4)
#define SMHC_RINT_RX_DATA_REQUEST	  (0x1 << 5)
#define SMHC_RINT_RESP_CRC_ERROR	  (0x1 << 6)
#define SMHC_RINT_DATA_CRC_ERROR	  (0x1 << 7)
#define SMHC_RINT_RESP_TIMEOUT		  (0x1 << 8)
#define SMHC_RINT_DATA_TIMEOUT		  (0x1 << 9)
#define SMHC_RINT_VOLTAGE_CHANGE_DONE (0x1 << 10)
#define SMHC_RINT_FIFO_RUN_ERROR	  (0x1 << 11)
#define SMHC_RINT_HARD_WARE_LOCKED	  (0x1 << 12)
#define SMHC_RINT_START_BIT_ERROR	  (0x1 << 13)
#define SMHC_RINT_AUTO_COMMAND_DONE	  (0x1 << 14)
#define SMHC_RINT_END_BIT_ERROR		  (0x1 << 15)
#define SMHC_RINT_SDIO_INTERRUPT	  (0x1 << 16)
#define SMHC_RINT_CARD_INSERT		  (0x1 << 30)
#define SMHC_RINT_CARD_REMOVE		  (0x1 << 31)
#define SMHC_RINT_INTERRUPT_ERROR_BIT                                                                                 \
	(SMHC_RINT_RESP_ERROR | SMHC_RINT_RESP_CRC_ERROR | SMHC_RINT_DATA_CRC_ERROR | SMHC_RINT_RESP_TIMEOUT |            \
	 SMHC_RINT_DATA_TIMEOUT | SMHC_RINT_VOLTAGE_CHANGE_DONE | SMHC_RINT_FIFO_RUN_ERROR | SMHC_RINT_HARD_WARE_LOCKED | \
	 SMHC_RINT_START_BIT_ERROR | SMHC_RINT_END_BIT_ERROR) /* 0xbfc2 */
#define SMHC_RINT_INTERRUPT_DONE_BIT \
	(SMHC_RINT_AUTO_COMMAND_DONE | SMHC_RINT_DATA_OVER | SMHC_RINT_COMMAND_DONE | SMHC_RINT_VOLTAGE_CHANGE_DONE)

/*
 * Status
 */
#define SMHC_STATUS_RXWL_FLAG	   (1 << 0)
#define SMHC_STATUS_TXWL_FLAG	   (1 << 1)
#define SMHC_STATUS_FIFO_EMPTY	   (1 << 2)
#define SMHC_STATUS_FIFO_FULL	   (1 << 3)
#define SMHC_STATUS_CARD_PRESENT   (1 << 8)
#define SMHC_STATUS_CARD_DATA_BUSY (1 << 9)
#define SMHC_STATUS_DATA_FSM_BUSY  (1 << 10)
#define SMHC_STATUS_DMA_REQUEST	   (1 << 31)
#define SMHC_STATUS_FIFO_SIZE	   (16)
#define SMHC_STATUS_FIFO_LEVEL(x)  (((x) >> 17) & 0x3fff)

/* IDMA controller bus mod bit field */
#define SMHC_IDMAC_SOFT_RESET  BIT(0)
#define SMHC_IDMAC_FIX_BURST   BIT(1)
#define SMHC_IDMAC_IDMA_ON	   BIT(7)
#define SMHC_IDMAC_REFETCH_DES BIT(31)

/* IDMA status bit field */
#define SMHC_IDMAC_TRANSMIT_INTERRUPT	  BIT(0)
#define SMHC_IDMAC_RECEIVE_INTERRUPT	  BIT(1)
#define SMHC_IDMAC_FATAL_BUS_ERROR		  BIT(2)
#define SMHC_IDMAC_DESTINATION_INVALID	  BIT(4)
#define SMHC_IDMAC_CARD_ERROR_SUM		  BIT(5)
#define SMHC_IDMAC_NORMAL_INTERRUPT_SUM	  BIT(8)
#define SMHC_IDMAC_ABNORMAL_INTERRUPT_SUM BIT(9)
#define SMHC_IDMAC_HOST_ABORT_INTERRUPT	  BIT(10)
#define SMHC_IDMAC_IDLE					  (0 << 13)
#define SMHC_IDMAC_SUSPEND				  (1 << 13)
#define SMHC_IDMAC_DESC_READ			  (2 << 13)
#define SMHC_IDMAC_DESC_CHECK			  (3 << 13)
#define SMHC_IDMAC_READ_REQUEST_WAIT	  (4 << 13)
#define SMHC_IDMAC_WRITE_REQUEST_WAIT	  (5 << 13)
#define SMHC_IDMAC_READ					  (6 << 13)
#define SMHC_IDMAC_WRITE				  (7 << 13)
#define SMHC_IDMAC_DESC_CLOSE			  (8 << 13)

/*
 * If the idma-des-size-bits of property is ie 13, bufsize bits are:
 *  Bits  0-12: buf1 size
 *  Bits 13-25: buf2 size
 *  Bits 26-31: not used
 * Since we only ever set buf1 size, we can simply store it directly.
 */
#define SMHC_IDMAC_DES0_DIC BIT(1) /* disable interrupt on completion */
#define SMHC_IDMAC_DES0_LD	BIT(2) /* last descriptor */
#define SMHC_IDMAC_DES0_FD	BIT(3) /* first descriptor */
#define SMHC_IDMAC_DES0_CH	BIT(4) /* chain mode */
#define SMHC_IDMAC_DES0_ER	BIT(5) /* end of ring */
#define SMHC_IDMAC_DES0_CES BIT(30) /* card error summary */
#define SMHC_IDMAC_DES0_OWN BIT(31) /* 1-idma owns it, 0-host owns it */

/*
timing mode
0: output and input are both based on [0,1,...,7] pll delay.
1: output and input are both based on phase.
2: output is based on phase, input is based on delay chain except hs400.
	input of hs400 is based on delay chain.
3: output is based on phase, input is based on delay chain.
4: output is based on phase, input is based on delay chain.
	it also support to use delay chain on data strobe signal.
*/
#define SUNXI_MMC_TIMING_MODE_0 0U
#define SUNXI_MMC_TIMING_MODE_1 1U
#define SUNXI_MMC_TIMING_MODE_2 2U
#define SUNXI_MMC_TIMING_MODE_3 3U
#define SUNXI_MMC_TIMING_MODE_4 4U
#define SUNXI_MMC_TIMING_MODE_5 5U

#define MMC_CLK_SAMPLE_POINIT_MODE_0	   8U
#define MMC_CLK_SAMPLE_POINIT_MODE_1	   3U
#define MMC_CLK_SAMPLE_POINIT_MODE_2	   2U
#define MMC_CLK_SAMPLE_POINIT_MODE_2_HS400 64U
#define MMC_CLK_SAMPLE_POINIT_MODE_3	   64U
#define MMC_CLK_SAMPLE_POINIT_MODE_4	   64U
#define MMC_CLK_SAMPLE_POINIT_MODE_5	   64U

#define TM5_OUT_PH90  (0)
#define TM5_OUT_PH180 (1)
#define TM5_IN_PH90	  (0)
#define TM5_IN_PH180  (1)
#define TM5_IN_PH270  (2)
#define TM5_IN_PH0	  (3)

/* delay control */
#define SDXC_NTDC_START_CAL	  (1 << 15)
#define SDXC_NTDC_CAL_DONE	  (1 << 14)
#define SDXC_NTDC_CAL_DLY	  (0x3F << 8)
#define SDXC_NTDC_ENABLE_DLY  (1 << 7)
#define SDXC_NTDC_CFG_DLY	  (0x3F << 0)
#define SDXC_NTDC_CFG_NEW_DLY (0xF << 0)

#define DTO_MAX						200
#define SUNXI_MMC_NTSR_MODE_SEL_NEW (0x1 << 31)

static void set_read_timeout(sdhci_t *sdhci, u32 timeout)
{
	u32 rval	 = 0;
	u32 rdto_clk = 0;
	u32 mode_2x	 = 0;

	rdto_clk = sdhci->clock / 1000 * timeout;
	rval	 = sdhci->reg->ntsr;
	mode_2x	 = rval & (0x1 << 31);

	if ((sdhci->clock == MMC_CLK_50M_DDR && mode_2x)) {
		rdto_clk = rdto_clk << 1;
	}

	rval = sdhci->reg->gctrl;
	/*ddr50 mode don't use 256x timeout unit*/
	if (rdto_clk > 0xffffff && sdhci->clock == MMC_CLK_50M_DDR) {
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

	trace("rdtoclk:%" PRIu32 ", reg-tmout:%" PRIu32 ", gctl:%" PRIx32 ", clock:%u, nstr:%" PRIx32 "\n", rdto_clk, sdhci->reg->timeout, sdhci->reg->gctrl,
		  sdhci->clock, sdhci->reg->ntsr);
}

static int prepare_dma(sdhci_t *sdhci, sdhci_data_t *data)
{
	sdhci_idma_desc_t *pdes		= sdhci->dma_desc;
	u32				   byte_cnt = data->blksz * data->blkcnt;
	u8				   *buff;
	u32				   des_idx		 = 0;
	u32				   buff_frag_num = 0;
	u32				   remain;
	u32				   i;

	buff		  = data->buf;
	buff_frag_num = byte_cnt >> SMHC_DES_NUM_SHIFT;
	remain		  = byte_cnt & (SMHC_DES_BUFFER_MAX_LEN - 1);
	if (remain)
		buff_frag_num++;
	else
		remain = SMHC_DES_BUFFER_MAX_LEN - 1;

	for (i = 0; i < buff_frag_num; i++, des_idx++) {
		memset((void *)&pdes[des_idx], 0, sizeof(sdhci_idma_desc_t));
		pdes[des_idx].des_chain = 1;
		pdes[des_idx].own		= 1;
		pdes[des_idx].dic		= 1;
		if (buff_frag_num > 1 && i != buff_frag_num - 1)
			pdes[des_idx].data_buf_sz = SMHC_DES_BUFFER_MAX_LEN - 1;
		else
			pdes[des_idx].data_buf_sz = remain;
		pdes[des_idx].buf_addr = ((u32)buff + i * SMHC_DES_BUFFER_MAX_LEN) >> 2;
		if (i == 0)
			pdes[des_idx].first_desc = 1;

		if (i == buff_frag_num - 1) {
			pdes[des_idx].dic			 = 0;
			pdes[des_idx].last_desc		 = 1;
			pdes[des_idx].next_desc_addr = 0;
		} else {
			pdes[des_idx].next_desc_addr = ((u32)&pdes[des_idx + 1]) >> 2;
		}
		trace("SMHC: frag %" PRIu32 ", remain %" PRIu32 ", des[%" PRIu32 "] = 0x%08" PRIx32 ":\r\n"
			  "  [0] = 0x%08" PRIx32 ", [1] = 0x%08" PRIx32 ", [2] = 0x%08" PRIx32 ", [3] = 0x%08" PRIx32 "\r\n",
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
	sdhci->reg->gctrl |= SMHC_GCTRL_DMA_ENABLE | SMHC_GCTRL_DMA_RESET; /* dma enable */
	sdhci->reg->dmac = SMHC_IDMAC_SOFT_RESET; /* idma reset */
	while (sdhci->reg->dmac & SMHC_IDMAC_SOFT_RESET) {
	} /* wait idma reset done */

	sdhci->reg->dmac = SMHC_IDMAC_FIX_BURST | SMHC_IDMAC_IDMA_ON; /* idma on */
	sdhci->reg->idie &= ~(SMHC_IDMAC_TRANSMIT_INTERRUPT | SMHC_IDMAC_RECEIVE_INTERRUPT);
	if (data->flag & MMC_DATA_WRITE)
		sdhci->reg->idie |= SMHC_IDMAC_TRANSMIT_INTERRUPT;
	else
		sdhci->reg->idie |= SMHC_IDMAC_RECEIVE_INTERRUPT;

	sdhci->reg->dlba	  = (u32)pdes >> 2;
	sdhci->reg->ftrglevel = sdhci->dma_trglvl;

	return 0;
}

static int wait_done(sdhci_t *sdhci, sdhci_data_t *dat, u32 timeout_msecs, u32 flag, bool dma)
{
	u32 status;
	u32 done  = 0;
	u32 start = time_ms();
	trace("SMHC: wait for flag 0x%" PRIx32 "\r\n", flag);
	do {
		status = sdhci->reg->rint;
		if ((time_ms() > (start + timeout_msecs))) {
			warning("SMHC: wait timeout %" PRIx32 " status %" PRIx32 " flag %" PRIx32 "\r\n", status & SMHC_RINT_INTERRUPT_ERROR_BIT, status,
					flag);
			return -1;
		} else if ((status & SMHC_RINT_INTERRUPT_ERROR_BIT)) {
			warning("SMHC: error 0x%" PRIx32 " status 0x%" PRIx32 "\r\n", status & SMHC_RINT_INTERRUPT_ERROR_BIT,
					status & ~SMHC_RINT_INTERRUPT_ERROR_BIT);
			return -1;
		}
		if (dat && dma && (dat->blkcnt * dat->blksz) > 0)
			done = ((status & flag) && (sdhci->reg->idst & SMHC_IDMAC_RECEIVE_INTERRUPT)) ? 1 : 0;
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
	u32	 in_fifo;

	if (timeout < 250)
		timeout = 250;

	trace("SMHC: read %" PRIu32 "\r\n", count);

	status = sdhci->reg->status;
	err	   = sdhci->reg->rint & SMHC_RINT_INTERRUPT_ERROR_BIT;
	if (err)
		warning("SMHC: interrupt error 0x%" PRIx32 " status 0x%" PRIx32 "\r\n", err & SMHC_RINT_INTERRUPT_ERROR_BIT, status);

	while ((!err) && (count >= sizeof(sdhci->reg->fifo))) {
		while (sdhci->reg->status & SMHC_STATUS_FIFO_EMPTY) {
			if (time_ms() > timeout) {
				warning("SMHC: read timeout\r\n");
				return FALSE;
			}
		}
		in_fifo = SMHC_STATUS_FIFO_LEVEL(status);
		count -= sizeof(sdhci->reg->fifo) * in_fifo;
		while (in_fifo--) {
			*(tmp++) = sdhci->reg->fifo;
		}

		status = sdhci->reg->status;
		err	   = sdhci->reg->rint & SMHC_RINT_INTERRUPT_ERROR_BIT;
	}

	do {
		status = sdhci->reg->rint;

		err = status & SMHC_RINT_INTERRUPT_ERROR_BIT;
		if (dat->blkcnt > 1)
			done = status & SMHC_RINT_AUTO_COMMAND_DONE;
		else
			done = status & SMHC_RINT_DATA_OVER;

	} while (!done && !err);

	if (err & SMHC_RINT_INTERRUPT_ERROR_BIT) {
		warning("SMHC: interrupt error 0x%" PRIx32 " status 0x%" PRIx32 "\r\n", err & SMHC_RINT_INTERRUPT_ERROR_BIT, status);
		return FALSE;
	}

	if (count) {
		warning("SMHC: read %" PRIu32 " leftover\r\n", count);
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

	trace("SMHC: write %llu\r\n", count);

	status = sdhci->reg->status;
	err	   = sdhci->reg->rint & SMHC_RINT_INTERRUPT_ERROR_BIT;
	while (!err && count) {
		while (sdhci->reg->status & SMHC_STATUS_FIFO_FULL) {
			if (time_ms() > timeout) {
				warning("SMHC: write timeout\r\n");
				return FALSE;
			}
		}
		sdhci->reg->fifo = *(tmp++);
		count -= sizeof(u32);

		status = sdhci->reg->status;
		err	   = sdhci->reg->rint & SMHC_RINT_INTERRUPT_ERROR_BIT;
	}

	do {
		status = sdhci->reg->rint;
		err	   = status & SMHC_RINT_INTERRUPT_ERROR_BIT;
		if (dat->blkcnt > 1)
			done = status & SMHC_RINT_AUTO_COMMAND_DONE;
		else
			done = status & SMHC_RINT_DATA_OVER;
	} while (!done && !err);

	if (err & SMHC_RINT_INTERRUPT_ERROR_BIT)
		return FALSE;

	if (count)
		return FALSE;
	return TRUE;
}

bool sdhci_transfer(sdhci_t *sdhci, sdhci_cmd_t *cmd, sdhci_data_t *dat)
{
	u32	 cmdval = 0;
	u32	 status = 0;
	u32	 timeout;
	bool dma = false;

	trace("SMHC: CMD%" PRIu32 " 0x%" PRIx32 " dlen:%" PRIu32 "\r\n", cmd->idx, cmd->arg, dat ? dat->blkcnt * dat->blksz : 0);

	if (cmd->idx == MMC_STOP_TRANSMISSION) {
		timeout = time_ms();
		do {
			status = sdhci->reg->status;
			if (time_ms() - timeout > 10) {
				sdhci->reg->gctrl = SMHC_GCTRL_HARDWARE_RESET;
				sdhci->reg->rint  = 0xffffffff;
				warning("SMHC: stop timeout\r\n");
				return FALSE;
			}
		} while (status & SMHC_STATUS_CARD_DATA_BUSY);
		return TRUE;
	}

	if (cmd->resptype & MMC_RSP_PRESENT) {
		cmdval |= SMHC_CMD_RESP_EXPIRE;
		if (cmd->resptype & MMC_RSP_136)
			cmdval |= SMHC_CMD_LONG_RESPONSE;
		if (cmd->resptype & MMC_RSP_CRC)
			cmdval |= SMHC_CMD_CHECK_RESPONSE_CRC;
	}

	if (dat) {
		sdhci->reg->blksz	= dat->blksz;
		sdhci->reg->bytecnt = (u32)(dat->blkcnt * dat->blksz);

		cmdval |= SMHC_CMD_DATA_EXPIRE | SMHC_CMD_WAIT_PRE_OVER;
		set_read_timeout(sdhci, DTO_MAX);

		if (dat->flag & MMC_DATA_WRITE) {
			cmdval |= SMHC_CMD_WRITE;
			sdhci->reg->idst |= SMHC_IDMAC_TRANSMIT_INTERRUPT; // clear TX status
		}
		if (dat->flag & MMC_DATA_READ)
			sdhci->reg->idst |= SMHC_IDMAC_RECEIVE_INTERRUPT; // clear RX status
	}

	if (cmd->idx == MMC_WRITE_MULTIPLE_BLOCK || cmd->idx == MMC_READ_MULTIPLE_BLOCK)
		cmdval |= SMHC_CMD_SEND_AUTO_STOP;

	sdhci->reg->rint = 0xffffffff; // Clear status
	sdhci->reg->arg	 = cmd->arg;

	if (dat && (dat->blkcnt * dat->blksz) > 64) {
		dma = true;
		sdhci->reg->gctrl &= ~SMHC_GCTRL_ACCESS_BY_AHB;
		prepare_dma(sdhci, dat);
		sdhci->reg->cmd = cmdval | cmd->idx | SMHC_CMD_START; // Start
	} else if (dat && (dat->blkcnt * dat->blksz) > 0) {
		sdhci->reg->gctrl |= SMHC_GCTRL_ACCESS_BY_AHB;
		sdhci->reg->cmd = cmdval | cmd->idx | SMHC_CMD_START; // Start
		if (dat->flag & MMC_DATA_READ && !read_bytes(sdhci, dat))
			return FALSE;
		else if (dat->flag & MMC_DATA_WRITE && !write_bytes(sdhci, dat))
			return FALSE;
	} else {
		sdhci->reg->gctrl |= SMHC_GCTRL_ACCESS_BY_AHB;
		sdhci->reg->cmd = cmdval | cmd->idx | SMHC_CMD_START; // Start
	}

	if (wait_done(sdhci, 0, 100, SMHC_RINT_COMMAND_DONE, false)) {
		warning("SMHC: cmd timeout\r\n");
		return FALSE;
	}

	if (dat && wait_done(sdhci, dat, 6000, dat->blkcnt > 1 ? SMHC_RINT_AUTO_COMMAND_DONE : SMHC_RINT_DATA_OVER, dma)) {
		warning("SMHC: data timeout\r\n");
		return FALSE;
	}

	if (cmd->resptype & MMC_RSP_BUSY) {
		timeout = time_ms();
		do {
			status = sdhci->reg->status;
			if (time_ms() - timeout > 10) {
				sdhci->reg->gctrl = SMHC_GCTRL_HARDWARE_RESET;
				sdhci->reg->rint  = 0xffffffff;
				warning("SMHC: busy timeout\r\n");
				return FALSE;
			}
		} while (status & SMHC_STATUS_CARD_DATA_BUSY);
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
		sdhci->reg->gctrl &= ~SMHC_GCTRL_DMA_ENABLE;
	}

	return TRUE;
}

bool sdhci_reset(sdhci_t *sdhci)
{
	sdhci->reg->gctrl = SMHC_GCTRL_HARDWARE_RESET;
	return TRUE;
}

bool sdhci_set_width(sdhci_t *sdhci, u32 width)
{
	const char UNUSED_TRACE *mode = "1 bit";
	sdhci->reg->gctrl &= ~SMHC_GCTRL_DDR_MODE;
	switch (width) {
		case MMC_BUS_WIDTH_1:
			sdhci->reg->width = SMHC_WIDTH_1BIT;
			break;
		case MMC_BUS_WIDTH_4:
			sdhci->reg->width = SMHC_WIDTH_4BIT;
			mode			  = "4 bit";
			break;
		default:
			error("SMHC: %" PRIu32 " width value invalid\r\n", width);
			return FALSE;
	}
	if (sdhci->clock == MMC_CLK_50M_DDR) {
		sdhci->reg->gctrl |= SMHC_GCTRL_DDR_MODE;
		mode = "4 bit DDR";
	}

	trace("SMHC: set width to %s\r\n", mode);
	return TRUE;
}

static int init_default_timing(sdhci_t *sdhci)
{
	sdhci->odly[MMC_CLK_400K]	 = TM5_OUT_PH180;
	sdhci->odly[MMC_CLK_25M]	 = TM5_OUT_PH180;
	sdhci->odly[MMC_CLK_50M]	 = TM5_OUT_PH180;
	sdhci->odly[MMC_CLK_50M_DDR] = TM5_OUT_PH90;

	sdhci->sdly[MMC_CLK_400K]	 = TM5_IN_PH180;
	sdhci->sdly[MMC_CLK_25M]	 = TM5_IN_PH180;
	sdhci->sdly[MMC_CLK_50M]	 = TM5_IN_PH90;
	sdhci->sdly[MMC_CLK_50M_DDR] = TM5_IN_PH180;

	return 0;
}

static int config_delay(sdhci_t *sdhci)
{
	u32 rval, freq;
	u8	odly, sdly;

	freq = sdhci->clock;

	odly = sdhci->odly[freq];
	sdly = sdhci->sdly[freq];

	trace("SMHC: odly: %d   sldy: %d\r\n", odly, sdly);

	ccu->smhc0_clk_cfg &= (~CCU_MMC_CTRL_ENABLE);
	sdhci->reg->drv_dl &= (~(0x3 << 16));
	sdhci->reg->drv_dl |= (((odly & 0x1) << 16) | ((odly & 0x1) << 17));
	ccu->smhc0_clk_cfg |= CCU_MMC_CTRL_ENABLE;

	rval = sdhci->reg->ntsr;
	rval &= (~(0x3 << 8));
	rval |= ((sdly & 0x3) << 8);
	sdhci->reg->ntsr = rval;

	/*enable hw skew auto mode*/
	rval = sdhci->reg->skew_ctrl;
	rval |= (0x1 << 4);
	sdhci->reg->skew_ctrl = rval;

	return 0;
}

static bool update_card_clock(sdhci_t *sdhci)
{
	sdhci->reg->cmd = SMHC_CMD_START | SMHC_CMD_UPCLK_ONLY | SMHC_CMD_WAIT_PRE_OVER;
	u32 timeout		= time_ms();

	do {
		if (time_ms() - timeout > 10)
			return FALSE;
	} while (sdhci->reg->cmd & SMHC_CMD_START);

	sdhci->reg->rint = 0xffffffff;
	return TRUE;
}

bool sdhci_set_clock(sdhci_t *sdhci, smhc_clk_t clock)
{
	u32 div, n, mod_hz, pll, pll_hz, hz;

	switch (clock) {
		case MMC_CLK_400K:
			hz = 400000;
			break;
		case MMC_CLK_25M:
			hz = 25000000;
			break;
		case MMC_CLK_50M:
		case MMC_CLK_50M_DDR:
			hz = 50000000;
			break;
		case MMC_CLK_100M:
			hz = 100000000;
			break;
		case MMC_CLK_150M:
			hz = 150000000;
			break;
		case MMC_CLK_200M:
			hz = 200000000;
			break;
		default:
			error("SHMC: invalid clock: %u\r\n", clock);
			return false;
	}

	if (hz < 1000000) {
		trace("SMHC: set clock to %.2fKHz\r\n", (f32)((f32)hz / 1000.0));
	} else {
		trace("SMHC: set clock to %.2fMHz\r\n", (f32)((f32)hz / 1000000.0));
	}

	if (sdhci->clock == MMC_CLK_50M_DDR)
		mod_hz = hz * 4; /* 4xclk: DDR 4(HS); */
	else
		mod_hz = hz * 2; /* 2xclk: SDR 1/4; */

	if (mod_hz <= 24000000) {
		pll	   = CCU_MMC_CTRL_OSCM24;
		pll_hz = 24000000;
	} else {
		pll	   = CCU_MMC_CTRL_PLL_PERIPH1X;
		pll_hz = sunxi_clk_get_peri1x_rate(); // 600Mhz
	}

	div = pll_hz / mod_hz;
	if (pll_hz % mod_hz)
		div++;

	n = 0;
	while (div > 16) {
		n++;
		div = (div + 1) / 2;
	}

	if (n > 3) {
		error("mmc %s error cannot set clock to %" PRIu32 "Hz\n", sdhci->name, hz);
		return false;
	}

	trace("SMHC: clock ratio %" PRIu32 "\r\n", div);

	sdhci->reg->clkcr &= ~SMHC_CLKCR_CARD_CLOCK_ON; // Disable clock
	if (!update_card_clock(sdhci))
		return false;

	sdhci->reg->ntsr |= SUNXI_MMC_NTSR_MODE_SEL_NEW;

	ccu->smhc_gate_reset |= CCU_MMC_BGR_SMHC0_RST;
	ccu->smhc0_clk_cfg &= (~CCU_MMC_CTRL_ENABLE);
	ccu->smhc0_clk_cfg = pll | CCU_MMC_CTRL_N(n) | CCU_MMC_CTRL_M(div);
	ccu->smhc0_clk_cfg |= CCU_MMC_CTRL_ENABLE;
	ccu->smhc_gate_reset |= CCU_MMC_BGR_SMHC0_GATE;

	sdhci->pclk = mod_hz;

	sdhci->reg->clkcr |= SMHC_CLKCR_MASK_D0; // Mask D0 when updating
	sdhci->reg->clkcr &= ~(0xff); // Clear div (set to 1)
	if (sdhci->clock == MMC_CLK_50M_DDR) {
		sdhci->reg->clkcr |= SMHC_CLKCR_CLOCK_DIV(2);
	}
	sdhci->reg->clkcr |= SMHC_CLKCR_CARD_CLOCK_ON; // Enable clock

	if (!update_card_clock(sdhci))
		return false;

	config_delay(sdhci);

	return true;
}

int sunxi_sdhci_init(sdhci_t *sdhci)
{
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

	init_default_timing(sdhci);
	sdhci_set_clock(sdhci, MMC_CLK_400K);

	sdhci->reg->gctrl = SMHC_GCTRL_HARDWARE_RESET;
	sdhci->reg->rint  = 0xffffffff;

	sdhci->dma_trglvl = ((0x3 << 28) | (15 << 16) | 240);

	udelay(100);

	return 0;
}
