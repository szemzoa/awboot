/*
 * driver/sd/sdcard.c
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
#include "debug.h"
#include "sunxi_sdhci.h"
#include "sdcard.h"

#define FALSE 0
#define TRUE  1

sdcard_pdata_t sdcard0;

#define UNSTUFF_BITS(resp, start, size)                              \
	({                                                               \
		const int	   __size = size;                                \
		const uint32_t __mask = (__size < 32 ? 1 << __size : 0) - 1; \
		const int	   __off  = 3 - ((start) / 32);                  \
		const int	   __shft = (start)&31;                          \
		uint32_t	   __res;                                        \
                                                                     \
		__res = resp[__off] >> __shft;                               \
		if (__size + __shft > 32)                                    \
			__res |= resp[__off - 1] << ((32 - __shft) % 32);        \
		__res &__mask;                                               \
	})

static const unsigned tran_speed_unit[] = {
	[0] = 10000,
	[1] = 100000,
	[2] = 1000000,
	[3] = 10000000,
};

static const unsigned char tran_speed_time[] = {
	0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80,
};

static bool_t go_idle_state(sdhci_t *hci)
{
	sdhci_cmd_t cmd = {0};

	// CMD0 0x0 for user partition
	cmd.idx		 = MMC_GO_IDLE_STATE;
	cmd.arg		 = 0;
	cmd.resptype = hci->isspi ? MMC_RSP_R1 : MMC_RSP_NONE;

	return sdhci_transfer(hci, &cmd, NULL);
}

static bool_t sd_send_if_cond(sdhci_t *hci, sdcard_t *card)
{
	sdhci_cmd_t cmd = {0};

	cmd.idx = SD_CMD_SEND_IF_COND;
	if (hci->voltage & MMC_VDD_27_36)
		cmd.arg = (0x1 << 8);
	else if (hci->voltage & MMC_VDD_165_195)
		cmd.arg = (0x2 << 8);
	else
		cmd.arg = (0x0 << 8);
	cmd.arg |= 0xaa;
	cmd.resptype = MMC_RSP_R7;
	if (!sdhci_transfer(hci, &cmd, NULL))
		return FALSE;

	if ((cmd.response[0] & 0xff) != 0xaa)
		return FALSE;
	card->version = SD_VERSION_2;
	return TRUE;
}

static bool_t sd_send_op_cond(sdhci_t *hci, sdcard_t *card)
{
	sdhci_cmd_t cmd		= {0};
	int			retries = 50;

	if (!sd_send_if_cond(hci, card)) {
		return FALSE;
	}

	do {
		cmd.idx		 = MMC_APP_CMD;
		cmd.arg		 = 0;
		cmd.resptype = MMC_RSP_R1;
		if (!sdhci_transfer(hci, &cmd, NULL))
			continue;

		cmd.idx = SD_CMD_APP_SEND_OP_COND;
		if (hci->isspi) {
			cmd.arg = 0;
			if (card->version == SD_VERSION_2)
				cmd.arg |= OCR_HCS;
			cmd.resptype = MMC_RSP_R1;
			if (sdhci_transfer(hci, &cmd, NULL))
				break;
		} else {
			if (hci->voltage & MMC_VDD_27_36)
				cmd.arg = 0x00ff8000;
			else if (hci->voltage & MMC_VDD_165_195)
				cmd.arg = 0x00000080;
			else
				cmd.arg = 0;
			if (card->version == SD_VERSION_2)
				cmd.arg |= OCR_HCS;
			cmd.resptype = MMC_RSP_R3;
			if (!sdhci_transfer(hci, &cmd, NULL) || (cmd.response[0] & OCR_BUSY))
				break;
		}
	} while (retries--);

	if (retries <= 0)
		return FALSE;

	if (card->version != SD_VERSION_2)
		card->version = SD_VERSION_1_0;
	if (hci->isspi) {
		cmd.idx		 = MMC_SPI_READ_OCR;
		cmd.arg		 = 0;
		cmd.resptype = MMC_RSP_R3;
		if (!sdhci_transfer(hci, &cmd, NULL))
			return FALSE;
	}
	card->ocr			= cmd.response[0];
	card->high_capacity = ((card->ocr & OCR_HCS) == OCR_HCS);
	card->rca			= 0;

	return TRUE;
}

static bool_t mmc_send_op_cond(sdhci_t *hci, sdcard_t *card)
{
	sdhci_cmd_t cmd		= {0};
	int			retries = 100;

	do {
		cmd.idx			= MMC_SEND_OP_COND;
		cmd.resptype	= MMC_RSP_R3;
		cmd.arg			= 0xC0FF8080; // size > 2GB mode, mandatory for Samsung
		cmd.response[0] = 0;
		if (!sdhci_transfer(hci, &cmd, NULL)) {
			return FALSE;
		}
		udelay(5000);
	} while (!(cmd.response[0] & OCR_BUSY) && retries--);

	if (retries <= 0)
		return FALSE;

	if (hci->isspi) {
		cmd.idx		 = MMC_SPI_READ_OCR;
		cmd.arg		 = 0;
		cmd.resptype = MMC_RSP_R3;
		if (!sdhci_transfer(hci, &cmd, NULL))
			return FALSE;
	}
	card->version		= MMC_VERSION_UNKNOWN;
	card->ocr			= cmd.response[0];
	card->high_capacity = ((card->ocr & OCR_HCS) == OCR_HCS);
	card->rca			= 1;
	return TRUE;
}

static int mmc_status(sdhci_t *hci, sdcard_t *card)
{
	sdhci_cmd_t cmd		= {0};
	int			retries = 100;

	cmd.idx		 = MMC_SEND_STATUS;
	cmd.resptype = MMC_RSP_R1;
	cmd.arg		 = card->rca << 16;
	do {
		if (!sdhci_transfer(hci, &cmd, NULL))
			continue;
		if (cmd.response[0] & (1 << 8))
			break;
		udelay(1);
	} while (retries-- > 0);
	if (retries > 0)
		return ((cmd.response[0] >> 9) & 0xf);
	warning("SMHC: status error\r\n");
	return -1;
}

uint64_t mmc_read_blocks(sdhci_t *hci, sdcard_t *card, uint8_t *buf, uint64_t start, uint64_t blkcnt)
{
	sdhci_cmd_t	 cmd = {0};
	sdhci_data_t dat = {0};
	int			 status;

	if (blkcnt > 1)
		cmd.idx = MMC_READ_MULTIPLE_BLOCK;
	else
		cmd.idx = MMC_READ_SINGLE_BLOCK;
	if (card->high_capacity)
		cmd.arg = start;
	else
		cmd.arg = start * card->read_bl_len;
	cmd.resptype = MMC_RSP_R1;
	dat.buf		 = buf;
	dat.flag	 = MMC_DATA_READ;
	dat.blksz	 = card->read_bl_len;
	dat.blkcnt	 = blkcnt;

	if (!sdhci_transfer(hci, &cmd, &dat)) {
		warning("SMHC: read failed\r\n");
		return 0;
	}

	if (!hci->isspi) {
		do {
			status = mmc_status(hci, card);
			if (status < 0) {
				return 0;
			}
		} while ((status != MMC_STATUS_TRAN) && (status != MMC_STATUS_DATA));
	}

	if (blkcnt > 1) {
		cmd.idx		 = MMC_STOP_TRANSMISSION;
		cmd.arg		 = 0;
		cmd.resptype = MMC_RSP_R1B;
		if (!sdhci_transfer(hci, &cmd, NULL)) {
			warning("SMHC: transfer stop failed\r\n");
			return 0;
		}
	}
	return blkcnt;
}

static bool_t sdcard_detect(sdhci_t *hci, sdcard_t *card)
{
	sdhci_cmd_t	 cmd = {0};
	sdhci_data_t dat = {0};
	uint64_t	 csize, cmult;
	uint32_t	 unit, time;
	int			 width;
	int			 status;

	sdhci_reset(hci);
	sdhci_set_clock(hci, 400 * 1000);
	sdhci_set_width(hci, MMC_BUS_WIDTH_1);

	if (!go_idle_state(hci)) {
		error("SMHC: set idle state failed\r\n");
		return FALSE;
	}
	udelay(5000); // 1ms + 74 clocks @ 400KHz

	if (!sd_send_op_cond(hci, card)) {
		debug("SMHC: SD detect failed\r\n");

		sdhci_reset(hci);
		sdhci_set_clock(hci, 400 * 1000);
		sdhci_set_width(hci, MMC_BUS_WIDTH_1);

		if (!mmc_send_op_cond(hci, card)) {
			debug("SMHC: MMC detect failed\r\n");
			return FALSE;
		}
	}

	if (hci->isspi) {
		cmd.idx		 = MMC_SEND_CID;
		cmd.arg		 = 0;
		cmd.resptype = MMC_RSP_R1;
		if (!sdhci_transfer(hci, &cmd, NULL))
			return FALSE;
		card->cid[0] = cmd.response[0];
		card->cid[1] = cmd.response[1];
		card->cid[2] = cmd.response[2];
		card->cid[3] = cmd.response[3];

		cmd.idx		 = MMC_SEND_CSD;
		cmd.arg		 = 0;
		cmd.resptype = MMC_RSP_R1;
		if (!sdhci_transfer(hci, &cmd, NULL))
			return FALSE;
		card->csd[0] = cmd.response[0];
		card->csd[1] = cmd.response[1];
		card->csd[2] = cmd.response[2];
		card->csd[3] = cmd.response[3];
	} else {
		cmd.idx		 = MMC_ALL_SEND_CID;
		cmd.arg		 = 0;
		cmd.resptype = MMC_RSP_R2;
		if (!sdhci_transfer(hci, &cmd, NULL))
			return FALSE;
		card->cid[0] = cmd.response[0];
		card->cid[1] = cmd.response[1];
		card->cid[2] = cmd.response[2];
		card->cid[3] = cmd.response[3];

		cmd.idx		 = SD_CMD_SEND_RELATIVE_ADDR;
		cmd.arg		 = card->rca << 16;
		cmd.resptype = MMC_RSP_R6;
		if (!sdhci_transfer(hci, &cmd, NULL))
			return FALSE;
		if (card->version & SD_VERSION_SD)
			card->rca = (cmd.response[0] >> 16) & 0xffff;

		cmd.idx		 = MMC_SEND_CSD;
		cmd.arg		 = card->rca << 16;
		cmd.resptype = MMC_RSP_R2;
		if (!sdhci_transfer(hci, &cmd, NULL))
			return FALSE;
		card->csd[0] = cmd.response[0];
		card->csd[1] = cmd.response[1];
		card->csd[2] = cmd.response[2];
		card->csd[3] = cmd.response[3];

		cmd.idx		 = MMC_SELECT_CARD;
		cmd.arg		 = card->rca << 16;
		cmd.resptype = MMC_RSP_R1;
		if (!sdhci_transfer(hci, &cmd, NULL))
			return FALSE;
		do {
			status = mmc_status(hci, card);
			if (status < 0)
				return FALSE;
		} while (status != MMC_STATUS_TRAN);
	}

	if (card->version == MMC_VERSION_UNKNOWN) {
		switch ((card->csd[0] >> 26) & 0xf) {
			case 0:
				card->version = MMC_VERSION_1_2;
				break;
			case 1:
				card->version = MMC_VERSION_1_4;
				break;
			case 2:
				card->version = MMC_VERSION_2_2;
				break;
			case 3:
				card->version = MMC_VERSION_3;
				break;
			case 4:
				card->version = MMC_VERSION_4;
				break;
			default:
				card->version = MMC_VERSION_1_2;
				break;
		};
	}

	unit			 = tran_speed_unit[(card->csd[0] & 0x7)];
	time			 = tran_speed_time[((card->csd[0] >> 3) & 0xf)];
	card->tran_speed = time * unit;
	card->dsr_imp	 = UNSTUFF_BITS(card->csd, 76, 1);

	card->read_bl_len = 1 << UNSTUFF_BITS(card->csd, 80, 4);
	if (card->version & SD_VERSION_SD)
		card->write_bl_len = card->read_bl_len;
	else
		card->write_bl_len = 1 << ((card->csd[3] >> 22) & 0xf);
	if (card->read_bl_len > 512)
		card->read_bl_len = 512;
	if (card->write_bl_len > 512)
		card->write_bl_len = 512;

	if ((card->version & MMC_VERSION_MMC) && (card->version >= MMC_VERSION_4)) {
		cmd.idx		 = MMC_SEND_EXT_CSD;
		cmd.arg		 = 0;
		cmd.resptype = MMC_RSP_R1;
		dat.buf		 = card->extcsd;
		dat.flag	 = MMC_DATA_READ;
		dat.blksz	 = 512;
		dat.blkcnt	 = 1;
		if (!sdhci_transfer(hci, &cmd, &dat))
			return FALSE;
		if (!hci->isspi) {
			do {
				status = mmc_status(hci, card);
				if (status < 0)
					return FALSE;
			} while (status != MMC_STATUS_TRAN);
		}
		const char *strver = "unknown";
		switch (card->extcsd[192]) {
			case 1:
				card->version = MMC_VERSION_4_1;
				strver		  = "4.1";
				break;
			case 2:
				card->version = MMC_VERSION_4_2;
				strver		  = "4.2";
				break;
			case 3:
				card->version = MMC_VERSION_4_3;
				strver		  = "4.3";
				break;
			case 5:
				card->version = MMC_VERSION_4_41;
				strver		  = "4.41";
				break;
			case 6:
				card->version = MMC_VERSION_4_5;
				strver		  = "4.5";
				break;
			case 7:
				card->version = MMC_VERSION_5_0;
				strver		  = "5.0";
				break;
			case 8:
				card->version = MMC_VERSION_5_1;
				strver		  = "5.1";
				break;
			default:
				break;
		}
		debug("SMHC: MMC version %s\r\n", strver);
	}

	if (card->high_capacity) {
		if (card->version & SD_VERSION_SD) {
			csize		   = UNSTUFF_BITS(card->csd, 48, 22);
			card->capacity = (1 + csize) << 10;
		} else {
			card->capacity = card->extcsd[212] << 0 | card->extcsd[212 + 1] << 8 | card->extcsd[212 + 2] << 16 |
							 card->extcsd[212 + 3] << 24;
		}
	} else {
		cmult		   = UNSTUFF_BITS(card->csd, 47, 3);
		csize		   = UNSTUFF_BITS(card->csd, 62, 12);
		card->capacity = (csize + 1) << (cmult + 2);
	}
	card->capacity *= 1 << UNSTUFF_BITS(card->csd, 80, 4);
	debug("SMHC: capacity %luGB\r\n", (uint32_t)(card->capacity / 1024u / 1024u / 1024u));

	if (hci->isspi) {
		sdhci_set_clock(hci, min(card->tran_speed, hci->clock));
		sdhci_set_width(hci, MMC_BUS_WIDTH_1);
	} else {
		if (card->version & SD_VERSION_SD) {
			if ((hci->width & MMC_BUS_WIDTH_8) || (hci->width & MMC_BUS_WIDTH_4))
				width = 2;
			else
				width = 0;

			cmd.idx		 = MMC_APP_CMD;
			cmd.arg		 = card->rca << 16;
			cmd.resptype = MMC_RSP_R5;
			if (!sdhci_transfer(hci, &cmd, NULL))
				return FALSE;

			cmd.idx		 = SD_CMD_SWITCH_FUNC;
			cmd.arg		 = width;
			cmd.resptype = MMC_RSP_R1;
			if (!sdhci_transfer(hci, &cmd, NULL))
				return FALSE;
		} else if (card->version & MMC_VERSION_MMC) {
			if (hci->width & MMC_BUS_WIDTH_8)
				width = 2;
			else if (hci->width & MMC_BUS_WIDTH_4)
				width = 1;
			else
				width = 0;

			// Write EXT_CSD register 183 (width) with our value
			cmd.idx		 = SD_CMD_SWITCH_FUNC;
			cmd.resptype = MMC_RSP_R1;
			cmd.arg		 = (3 << 24) | (183 << 16) | (width << 8) | 1;
			if (!sdhci_transfer(hci, &cmd, NULL))
				return FALSE;

			udelay(1000);
		}
		sdhci_set_clock(hci, min(card->tran_speed, hci->clock));
		if ((hci->width & MMC_BUS_WIDTH_8) || (hci->width & MMC_BUS_WIDTH_4))
			sdhci_set_width(hci, MMC_BUS_WIDTH_4);
		else
			sdhci_set_width(hci, MMC_BUS_WIDTH_1);
	}

	cmd.idx		 = MMC_SET_BLOCKLEN;
	cmd.arg		 = card->read_bl_len;
	cmd.resptype = MMC_RSP_R1;
	if (!sdhci_transfer(hci, &cmd, NULL))
		return FALSE;

	return TRUE;
}

uint64_t sdcard_blk_read(sdcard_pdata_t *data, uint8_t *buf, uint64_t blkno, uint64_t blkcnt)
{
	uint64_t  cnt, blks = blkcnt;
	sdcard_t *sdcard = &data->card;

	while (blks > 0) {
		cnt = (blks > 127) ? 127 : blks;
		if (mmc_read_blocks(data->hci, sdcard, buf, blkno, cnt) != cnt)
			return 0;
		blks -= cnt;
		blkno += cnt;
		buf += cnt * sdcard->read_bl_len;
	}
	return blkcnt;
}

int sdcard_init(sdcard_pdata_t *data, sdhci_t *hci)
{
	data->hci	 = hci;
	data->online = FALSE;

	if (sdcard_detect(data->hci, &data->card) == TRUE) {
		info("SHMC: %s card detected\r\n", data->card.version & SD_VERSION_SD ? "SD" : "MMC");
		return 0;
	}

	return -1;
}
