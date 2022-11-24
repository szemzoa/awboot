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
#include "div.h"
#include "sdcard.h"
#include "debug.h"
#include "sunxi_sdhci.h"
#include "sunxi_gpio.h"
#include "sunxi_clk.h"

#define debug2(...)

#define FALSE 0
#define TRUE  1

enum {
	SD_GCTL		 = 0x00,
	SD_CKCR		 = 0x04,
	SD_TMOR		 = 0x08,
	SD_BWDR		 = 0x0c,
	SD_BKSR		 = 0x10,
	SD_BYCR		 = 0x14,
	SD_CMDR		 = 0x18,
	SD_CAGR		 = 0x1c,
	SD_RESP0	 = 0x20,
	SD_RESP1	 = 0x24,
	SD_RESP2	 = 0x28,
	SD_RESP3	 = 0x2c,
	SD_IMKR		 = 0x30,
	SD_MISR		 = 0x34,
	SD_RISR		 = 0x38,
	SD_STAR		 = 0x3c,
	SD_FWLR		 = 0x40,
	SD_FUNS		 = 0x44,
	SD_A12A		 = 0x58,
	SD_NTSR		 = 0x5c,
	SD_SDBG		 = 0x60,
	SD_HWRST	 = 0x78,
	SD_DMAC		 = 0x80,
	SD_DLBA		 = 0x84,
	SD_IDST		 = 0x88,
	SD_IDIE		 = 0x8c,
	SD_THLDC	 = 0x100,
	SD_DSBD		 = 0x10c,
	SD_RES_CRC	 = 0x110,
	SD_DATA7_CRC = 0x114,
	SD_DATA6_CRC = 0x118,
	SD_DATA5_CRC = 0x11c,
	SD_DATA4_CRC = 0x120,
	SD_DATA3_CRC = 0x124,
	SD_DATA2_CRC = 0x128,
	SD_DATA1_CRC = 0x12c,
	SD_DATA0_CRC = 0x130,
	SD_CRC_STA	 = 0x134,
	SD_FIFO		 = 0x200,
};

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
#define SDXC_WIDTH8 (2)

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

extern uint32_t get_sys_ticks();

static bool_t t113_transfer_command(sdhci_t *pdat, sdhci_cmd_t *cmd, sdhci_data_t *dat)
{
	uint32_t cmdval = SDXC_START;
	uint32_t status = 0;
	uint32_t timeout;

	trace("SMHC: CMD%u 0x%x dlen:%u\r\n", cmd->idx, cmd->arg, dat ? dat->blkcnt * dat->blksz : 0);

	if (cmd->idx == MMC_STOP_TRANSMISSION) {
		timeout = get_sys_ticks();
		do {
			status = read32(pdat->addr + SD_STAR);
			if (get_sys_ticks() - timeout > 10) {
				write32(pdat->addr + SD_GCTL, SDXC_HARDWARE_RESET);
				write32(pdat->addr + SD_RISR, 0xffffffff);
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

	if (dat)
		cmdval |= SDXC_DATA_EXPIRE | SDXC_WAIT_PRE_OVER;

	if (dat && (dat->flag & MMC_DATA_WRITE))
		cmdval |= SDXC_WRITE;

	if (cmd->idx == MMC_WRITE_MULTIPLE_BLOCK || cmd->idx == MMC_READ_MULTIPLE_BLOCK)
		cmdval |= SDXC_SEND_AUTO_STOP;

	write32(pdat->addr + SD_CAGR, cmd->arg);

	if (dat)
		write32(pdat->addr + SD_GCTL, read32(pdat->addr + SD_GCTL) | 0x80000000);
	write32(pdat->addr + SD_CMDR, cmdval | cmd->idx);

	timeout = get_sys_ticks();
	do {
		status = read32(pdat->addr + SD_RISR);
		if ((get_sys_ticks() - timeout > 10) || (status & SDXC_INTERRUPT_ERROR_BIT)) {
			write32(pdat->addr + SD_GCTL, SDXC_HARDWARE_RESET);
			write32(pdat->addr + SD_RISR, 0xffffffff);
			warning("SMHC: timeout\r\n");
			return FALSE;
		}
	} while (!(status & SDXC_COMMAND_DONE));

	if (cmd->resptype & MMC_RSP_BUSY) {
		timeout = get_sys_ticks();
		do {
			status = read32(pdat->addr + SD_STAR);
			if (get_sys_ticks() - timeout > 10) {
				write32(pdat->addr + SD_GCTL, SDXC_HARDWARE_RESET);
				write32(pdat->addr + SD_RISR, 0xffffffff);
				warning("SMHC: response timeout\r\n");
				return FALSE;
			}
		} while (status & (1 << 9));
	}

	if (cmd->resptype & MMC_RSP_136) {
		cmd->response[0] = read32(pdat->addr + SD_RESP3);
		cmd->response[1] = read32(pdat->addr + SD_RESP2);
		cmd->response[2] = read32(pdat->addr + SD_RESP1);
		cmd->response[3] = read32(pdat->addr + SD_RESP0);
	} else {
		cmd->response[0] = read32(pdat->addr + SD_RESP0);
	}

	write32(pdat->addr + SD_RISR, 0xffffffff);

	return TRUE;
}

static bool_t read_bytes(sdhci_t *pdat, uint32_t *buf, uint32_t blkcount, uint32_t blksize)
{
	uint64_t  count = blkcount * blksize;
	uint32_t *tmp	= buf;
	uint32_t  status, err, done;

	trace("SMHC: read %u\r\n", blkcount * blksize);

	status = read32(pdat->addr + SD_STAR);
	err	   = read32(pdat->addr + SD_RISR) & SDXC_INTERRUPT_ERROR_BIT;
	if (err)
		warning("SMHC: interrupt error 0x%x status 0x%x\r\n", err & SDXC_INTERRUPT_ERROR_BIT, status);

	while ((!err) && (count >= sizeof(uint32_t))) {
		if (!(status & SDXC_FIFO_EMPTY)) {
			*(tmp) = read32(pdat->addr + SD_FIFO);
			tmp++;
			count -= sizeof(uint32_t);
		}
		status = read32(pdat->addr + SD_STAR);
		err	   = read32(pdat->addr + SD_RISR) & SDXC_INTERRUPT_ERROR_BIT;
	}

	do {
		status = read32(pdat->addr + SD_RISR);

		err = status & SDXC_INTERRUPT_ERROR_BIT;
		if (blkcount > 1)
			done = status & SDXC_AUTO_COMMAND_DONE;
		else
			done = status & SDXC_DATA_OVER;

	} while (!done && !err);

	if (err & SDXC_INTERRUPT_ERROR_BIT) {
		warning("SMHC: interrupt error 0x%x status 0x%x\r\n", err & SDXC_INTERRUPT_ERROR_BIT, status);
		return FALSE;
	}
	write32(pdat->addr + SD_RISR, 0xffffffff);

	if (count) {
		warning("SMHC: read %u leftover\r\n", count);
		return FALSE;
	}
	return TRUE;
}

static bool_t write_bytes(sdhci_t *pdat, uint32_t *buf, uint32_t blkcount, uint32_t blksize)
{
	uint64_t  count = blkcount * blksize;
	uint32_t *tmp	= buf;
	uint32_t  status, err, done;

	trace("SMHC: write %u\r\n", blkcount);

	status = read32(pdat->addr + SD_STAR);
	err	   = read32(pdat->addr + SD_RISR) & SDXC_INTERRUPT_ERROR_BIT;
	while (!err && count) {
		if (!(status & SDXC_FIFO_FULL)) {
			write32(pdat->addr + SD_FIFO, *tmp);
			tmp++;
			count -= sizeof(uint32_t);
		}
		status = read32(pdat->addr + SD_STAR);
		err	   = read32(pdat->addr + SD_RISR) & SDXC_INTERRUPT_ERROR_BIT;
	}

	do {
		status = read32(pdat->addr + SD_RISR);
		err	   = status & SDXC_INTERRUPT_ERROR_BIT;
		if (blkcount > 1)
			done = status & SDXC_AUTO_COMMAND_DONE;
		else
			done = status & SDXC_DATA_OVER;
	} while (!done && !err);

	if (err & SDXC_INTERRUPT_ERROR_BIT)
		return FALSE;
	write32(pdat->addr + SD_GCTL, read32(pdat->addr + SD_GCTL) | SDXC_FIFO_RESET);
	write32(pdat->addr + SD_RISR, 0xffffffff);

	if (count)
		return FALSE;
	return TRUE;
}

static bool_t t113_transfer_data(sdhci_t *pdat, sdhci_cmd_t *cmd, sdhci_data_t *dat)
{
	uint32_t dlen = (uint32_t)(dat->blkcnt * dat->blksz);
	bool_t	 ret  = FALSE;

	write32(pdat->addr + SD_BKSR, dat->blksz);
	write32(pdat->addr + SD_BYCR, dlen);

	if (dat->flag & MMC_DATA_READ) {
		if (!t113_transfer_command(pdat, cmd, dat))
			return FALSE;
		ret = read_bytes(pdat, (uint32_t *)dat->buf, dat->blkcnt, dat->blksz);
	} else if (dat->flag & MMC_DATA_WRITE) {
		if (!t113_transfer_command(pdat, cmd, dat))
			return FALSE;
		ret = write_bytes(pdat, (uint32_t *)dat->buf, dat->blkcnt, dat->blksz);
	}
	return ret;
}

bool_t sdhci_reset(sdhci_t *sdhci)
{
	write32(sdhci->addr + SD_GCTL, SDXC_HARDWARE_RESET);
	return TRUE;
}

bool_t sdhci_set_width(sdhci_t *sdhci, uint32_t width)
{
	switch (width) {
		case MMC_BUS_WIDTH_1:
			write32(sdhci->addr + SD_BWDR, SDXC_WIDTH1);
			break;
		case MMC_BUS_WIDTH_4:
			write32(sdhci->addr + SD_BWDR, SDXC_WIDTH4);
			break;
		case MMC_BUS_WIDTH_8:
			write32(sdhci->addr + SD_BWDR, SDXC_WIDTH8);
			break;
		default:
			error("SMHC: %u bit width invalid\r\n", width);
			return FALSE;
	}
	trace("SMHC: set %u bit mode\r\n", width);
	return TRUE;
}

static bool_t sdhci_t113_update_clk(sdhci_t *pdat)
{
	uint32_t cmd = (1U << 31) | (1 << 21) | (1 << 13);

	write32(pdat->addr + SD_CMDR, cmd);
	uint32_t timeout = get_sys_ticks();
	do {
		if (get_sys_ticks() - timeout > 10)
			return FALSE;
	} while (read32(pdat->addr + SD_CMDR) & 0x80000000);
	write32(pdat->addr + SD_RISR, read32(pdat->addr + SD_RISR));
	return TRUE;
}

bool_t sdhci_set_clock(sdhci_t *sdhci, uint32_t clock)
{
	uint32_t ratio = div(sdhci->pclk + 2 * clock - 1, (2 * clock));

	trace("SMHC: set clock at %uHz\r\n", clock);

	if ((ratio & 0xff) != ratio)
		return FALSE;
	write32(sdhci->addr + SD_CKCR, read32(sdhci->addr + SD_CKCR) & ~(1 << 16));
	write32(sdhci->addr + SD_CKCR, ratio);
	if (!sdhci_t113_update_clk(sdhci))
		return FALSE;
	write32(sdhci->addr + SD_CKCR, read32(sdhci->addr + SD_CKCR) | (3 << 16));
	if (!sdhci_t113_update_clk(sdhci))
		return FALSE;
	return TRUE;
}

bool_t sdhci_transfer(sdhci_t *sdhci, sdhci_cmd_t *cmd, sdhci_data_t *dat)
{
	if (!dat)
		return t113_transfer_command(sdhci, cmd, dat);

	return t113_transfer_data(sdhci, cmd, dat);
}

int sunxi_sdhci_init(sdhci_t *sdhci)
{
	uint32_t addr, val;

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

	addr = 0x0200184c;
	val	 = read32(addr);
	val |= (1 << 16);
	write32(addr, val);

	/* Open the clock gate for sdhci0 */
	addr = 0x02001830;
	val	 = read32(addr);
	val |= (1 << 31) | (1 << 24) | 11; /* 50MHz */
	write32(addr, val);

	sdhci->pclk = div(sunxi_clk_get_peri1x_rate(), 12);

	/* sdhc0 clock gate pass */
	addr = 0x0200184c;
	val	 = read32(addr);
	val |= (1 << 0);
	write32(addr, val);

	udelay(10);

	write32(sdhci->addr + SD_GCTL, SDXC_HARDWARE_RESET);
	write32(sdhci->addr + SD_RISR, 0xffffffff);

	udelay(100);

	return 0;
}
