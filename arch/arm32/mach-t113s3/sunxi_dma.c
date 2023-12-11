/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <main.h>
#include "sunxi_dma.h"
#include "debug.h"
#include "reg-ccu.h"
#include "board.h"
#include "io.h"

#define SUNXI_DMA_MAX 16

static int			dma_int_cnt = 0;
static int			dma_init_ok = -1;
static dma_source_t dma_channel_source[SUNXI_DMA_MAX];
static dma_desc_t	dma_channel_desc[SUNXI_DMA_MAX] __attribute__((aligned(64)));

void dma_init(void)
{
	debug("DMA: init\r\n");
	int				 i;
	dma_reg_t *const dma_reg = (dma_reg_t *)SUNXI_DMA_BASE;

	if (dma_init_ok > 0)
		return;

	/* dma : mbus clock gating */
	ccu->mbus_gate |= 1 << 0;

	/* dma reset */
	ccu->dma_gate_reset |= 1 << DMA_RST_OFS;

	/* dma gating */
	ccu->dma_gate_reset |= 1 << DMA_GATING_OFS;

	dma_reg->irq_en0 = 0;
	dma_reg->irq_en1 = 0;

	dma_reg->irq_pending0 = 0xffffffff;
	dma_reg->irq_pending1 = 0xffffffff;

	/* auto MCLK gating  disable */
	dma_reg->auto_gate &= ~(0x7 << 0);
	dma_reg->auto_gate |= 0x7 << 0;

	memset((void *)dma_channel_source, 0, SUNXI_DMA_MAX * sizeof(dma_source_t));

	for (i = 0; i < SUNXI_DMA_MAX; i++) {
		dma_channel_source[i].used	  = 0;
		dma_channel_source[i].channel = &(dma_reg->channel[i]);
		dma_channel_source[i].desc	  = &dma_channel_desc[i];
	}

	dma_int_cnt = 0;
	dma_init_ok = 1;

	return;
}

void dma_exit(void)
{
	int		   i;
	dma_reg_t *dma_reg = (dma_reg_t *)SUNXI_DMA_BASE;
#if defined(CONFIG_SUNXI_VERSION1)
	struct sunxi_ccu_reg *const ccu = (struct sunxi_ccu_reg *)T113_CCU_BASE;
#endif
	/* free dma channel if other module not free it */
	for (i = 0; i < SUNXI_DMA_MAX; i++) {
		if (dma_channel_source[i].used == 1) {
			dma_channel_source[i].channel->enable = 0;
			dma_channel_source[i].used			  = 0;
		}
	}

#if defined(CONFIG_SUNXI_VERSION1)
	ccu->ahb_gate0 &= ~(1 << AHB_GATE_OFFSET_DMA);
#else
	/* close dma clock when dma exit */
	dma_reg->auto_gate &= ~(1 << DMA_GATING_OFS | 1 << DMA_RST_OFS);
#endif
        dma_reg->irq_en0 = 0;
        dma_reg->irq_en1 = 0;

        dma_reg->irq_pending0 = 0xffffffff;
        dma_reg->irq_pending1 = 0xffffffff;

	dma_init_ok--;
}

u32 dma_request_from_last(u32 dmatype)
{
	int i;

	for (i = SUNXI_DMA_MAX - 1; i >= 0; i--) {
		if (dma_channel_source[i].used == 0) {
			dma_channel_source[i].used			= 1;
			dma_channel_source[i].channel_count = i;
			return (u32)&dma_channel_source[i];
		}
	}

	return 0;
}

u32 dma_request(u32 dmatype)
{
	int i;

	for (i = 0; i < SUNXI_DMA_MAX; i++) {
		if (dma_channel_source[i].used == 0) {
			dma_channel_source[i].used			= 1;
			dma_channel_source[i].channel_count = i;
			trace("DMA: provide channel %u\r\n", i);
			return (u32)&dma_channel_source[i];
		}
	}

	return 0;
}

int dma_release(u32 hdma)
{
	dma_source_t *dma_source = (dma_source_t *)hdma;

	if (!dma_source->used)
		return -1;

	dma_source->used = 0;

	return 0;
}

int dma_setting(u32 hdma, dma_set_t *cfg)
{
	u32			  commit_para;
	dma_set_t	  *dma_set	   = cfg;
	dma_source_t *dma_source   = (dma_source_t *)hdma;
	dma_desc_t   *desc		   = dma_source->desc;
	u32			  channel_addr = (u32)(&(dma_set->channel_cfg));

	if (!dma_source->used)
		return -1;

	if (dma_set->loop_mode)
		desc->link = (u32)(&dma_source->desc);
	else
		desc->link = SUNXI_DMA_LINK_NULL;

	commit_para = (dma_set->wait_cyc & 0xff);
	commit_para |= (dma_set->data_block_size & 0xff) << 8;

	desc->commit_para = commit_para;
	desc->config	  = *(volatile u32 *)channel_addr;

	return 0;
}

int dma_start(u32 hdma, u32 saddr, u32 daddr, u32 bytes)
{
	dma_source_t		 *dma_source = (dma_source_t *)hdma;
	dma_channel_reg_t *channel	  = dma_source->channel;
	dma_desc_t		   *desc		  = dma_source->desc;

	if (!dma_source->used)
		return -1;

	/*config desc */
	desc->source_addr = saddr;
	desc->dest_addr	  = daddr;
	desc->byte_count  = bytes;

	/* start dma */
	channel->desc_addr = (u32)desc;
	channel->enable	   = 1;

	return 0;
}

int dma_stop(u32 hdma)
{
	dma_source_t		 *dma_source = (dma_source_t *)hdma;
	dma_channel_reg_t *channel	  = dma_source->channel;

	if (!dma_source->used)
		return -1;
	channel->enable = 0;

	return 0;
}

int dma_querystatus(u32 hdma)
{
	u32			  channel_count;
	dma_source_t *dma_source = (dma_source_t *)hdma;
	dma_reg_t	  *dma_reg	 = (dma_reg_t *)SUNXI_DMA_BASE;

	if (!dma_source->used)
		return -1;

	channel_count = dma_source->channel_count;

	return (dma_reg->status >> channel_count) & 0x01;
}

int dma_test()
{
	u32		*src_addr = (u32 *)CONFIG_DTB_LOAD_ADDR;
	u32		*dst_addr = (u32 *)CONFIG_KERNEL_LOAD_ADDR;
	u32		  len	   = 512 * 1024;
	dma_set_t dma_set;
	u32		  hdma, st = 0;
	u32		  timeout;
	u32		  i, valid;

	len = ALIGN(len, 4);
	trace("DMA: test 0x%08" PRIx32 " ====> 0x%08" PRIx32 ", len %" PRIu32 "KB \r\n", (u32)src_addr, (u32)dst_addr, (len / 1024));

	/* dma */
	dma_set.loop_mode		= 0;
	dma_set.wait_cyc		= 8;
	dma_set.data_block_size = 1 * 32 / 8;
	/* channel config (from dram to dram)*/
	dma_set.channel_cfg.src_drq_type	 = DMAC_CFG_TYPE_DRAM; // dram
	dma_set.channel_cfg.src_addr_mode	 = DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE;
	dma_set.channel_cfg.src_burst_length = DMAC_CFG_SRC_4_BURST;
	dma_set.channel_cfg.src_data_width	 = DMAC_CFG_SRC_DATA_WIDTH_16BIT;
	dma_set.channel_cfg.reserved0		 = 0;

	dma_set.channel_cfg.dst_drq_type	 = DMAC_CFG_TYPE_DRAM; // dram
	dma_set.channel_cfg.dst_addr_mode	 = DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE;
	dma_set.channel_cfg.dst_burst_length = DMAC_CFG_DEST_4_BURST;
	dma_set.channel_cfg.dst_data_width	 = DMAC_CFG_DEST_DATA_WIDTH_16BIT;
	dma_set.channel_cfg.reserved1		 = 0;

	hdma = dma_request(0);
	if (!hdma) {
		error("DMA: can't request dma\r\n");
		return -1;
	}

	dma_setting(hdma, &dma_set);

	// prepare data
	for (i = 0; i < (len / 4); i += 4) {
		src_addr[i]		= i;
		src_addr[i + 1] = i + 1;
		src_addr[i + 2] = i + 2;
		src_addr[i + 3] = i + 3;
	}

	/* timeout : 100 ms */
	timeout = time_ms();

	dma_start(hdma, (u32)src_addr, (u32)dst_addr, len);
	st = dma_querystatus(hdma);

	while ((time_ms() - timeout < 100) && st) {
		st = dma_querystatus(hdma);
	}

	if (st) {
		error("DMA: test timeout!\r\n");
		dma_stop(hdma);
		dma_release(hdma);

		return -2;
	} else {
		valid = 1;
		// Check data is valid
		for (i = 0; i < (len / 4); i += 4) {
			if (dst_addr[i] != i || dst_addr[i + 1] != i + 1 || dst_addr[i + 2] != i + 2 || dst_addr[i + 3] != i + 3) {
				valid = 0;
				break;
			}
		}
		if (valid) {
			debug("DMA: test OK in %lums\r\n", (time_ms() - timeout));
		} else
			error("DMA: test check failed at %" PRIu32 " bytes\r\n", i);
	}

	dma_stop(hdma);
	dma_release(hdma);

	return 0;
}
