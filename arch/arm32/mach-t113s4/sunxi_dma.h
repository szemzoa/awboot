/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2015 Roy Spliet <rspliet@ultimaker.com>
 */

#include "types.h"

#ifndef _SUNXI_DMA_H
#define _SUNXI_DMA_H

#define SUNXI_DMA_BASE		   0x03002000
#define SUNXI_DMA_CHANNEL_BASE (SUNXI_DMA_BASE + 0x100)
#define DMA_AUTO_GATE_REG	   (SUNXI_DMA_BASE + 0x28)

#define SUNXI_DMA_CHANNEL_SIZE (0x40)
#define SUNXI_DMA_LINK_NULL	   (0xfffff800)

#define DMAC_DMATYPE_NORMAL 0
#define DMAC_CFG_TYPE_DRAM	(1)
#define DMAC_CFG_TYPE_SRAM	(0)

#define DMAC_CFG_TYPE_SPI0	   (22)
#define DMAC_CFG_TYPE_SHMC0	   (20)
#define DMAC_CFG_SRC_TYPE_NAND (5)

/* DMA base config  */
#define DMAC_CFG_CONTINUOUS_ENABLE	(0x01)
#define DMAC_CFG_CONTINUOUS_DISABLE (0x00)

/* ----------DMA dest config-------------------- */
/* DMA dest width config */
#define DMAC_CFG_DEST_DATA_WIDTH_8BIT  (0x00)
#define DMAC_CFG_DEST_DATA_WIDTH_16BIT (0x01)
#define DMAC_CFG_DEST_DATA_WIDTH_32BIT (0x02)
#define DMAC_CFG_DEST_DATA_WIDTH_64BIT (0x03)

/* DMA dest bust config */
#define DMAC_CFG_DEST_1_BURST  (0x00)
#define DMAC_CFG_DEST_4_BURST  (0x01)
#define DMAC_CFG_DEST_8_BURST  (0x02)
#define DMAC_CFG_DEST_16_BURST (0x03)

#define DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE (0x00)
#define DMAC_CFG_DEST_ADDR_TYPE_IO_MODE		(0x01)

/* ----------DMA src config -------------------*/
#define DMAC_CFG_SRC_DATA_WIDTH_8BIT  (0x00)
#define DMAC_CFG_SRC_DATA_WIDTH_16BIT (0x01)
#define DMAC_CFG_SRC_DATA_WIDTH_32BIT (0x02)
#define DMAC_CFG_SRC_DATA_WIDTH_64BIT (0x03)

#define DMAC_CFG_SRC_1_BURST  (0x00)
#define DMAC_CFG_SRC_4_BURST  (0x01)
#define DMAC_CFG_SRC_8_BURST  (0x02)
#define DMAC_CFG_SRC_16_BURST (0x03)

#define DMAC_CFG_SRC_ADDR_TYPE_LINEAR_MODE (0x00)
#define DMAC_CFG_SRC_ADDR_TYPE_IO_MODE	   (0x01)

/*dma int config*/
#define DMA_PKG_HALF_INT  (1 << 0)
#define DMA_PKG_END_INT	  (1 << 1)
#define DMA_QUEUE_END_INT (1 << 2)

typedef struct {
	u32 volatile config;
	u32 volatile source_addr;
	u32 volatile dest_addr;
	u32 volatile byte_count;
	u32 volatile commit_para;
	u32 volatile link;
	u32 volatile reserved[2];
} dma_desc_t;

typedef struct {
	u32 volatile src_drq_type : 6;
	u32 volatile src_burst_length : 2;
	u32 volatile src_addr_mode : 1;
	u32 volatile src_data_width : 2;
	u32 volatile reserved0 : 5;
	u32 volatile dst_drq_type : 6;
	u32 volatile dst_burst_length : 2;
	u32 volatile dst_addr_mode : 1;
	u32 volatile dst_data_width : 2;
	u32 volatile reserved1 : 5;
} dma_channel_config_t;

typedef struct {
	dma_channel_config_t channel_cfg;
	u32					 loop_mode;
	u32					 data_block_size;
	u32					 wait_cyc;
} dma_set_t;

typedef struct {
	void *m_data;
	// void (*m_func)(void *data);
	void (*m_func)(void);
} dma_irq_handler_t;

typedef struct {
	u32 volatile enable;
	u32 volatile pause;
	u32 volatile desc_addr;
	u32 volatile config;
	u32 volatile cur_src_addr;
	u32 volatile cur_dst_addr;
	u32 volatile left_bytes;
	u32 volatile parameters;
	u32 volatile mode;
	u32 volatile fdesc_addr;
	u32 volatile pkg_num;
	u32 volatile res[5];
} dma_channel_reg_t;

typedef struct {
	u32 volatile irq_en0; /* 0x0 dma irq enable register 0 */
	u32 volatile irq_en1; /* 0x4 dma irq enable register 1 */
	u32 volatile reserved0[2];
	u32 volatile irq_pending0; /* 0x10 dma irq pending register 0 */
	u32 volatile irq_pending1; /* 0x14 dma irq pending register 1 */
	u32 volatile reserved1[2];
	u32 volatile security; /* 0x20 dma security register */
	u32 volatile reserved3[1];
	u32 volatile auto_gate; /* 0x28 dma auto gating register */
	u32 volatile reserved4[1];
	u32 volatile status; /* 0x30 dma status register */
	u32 volatile reserved5[3];
	u32 volatile version; /* 0x40 dma Version register */
	u32 volatile reserved6[47];
	dma_channel_reg_t channel[16]; /* 0x100 dma channel register */
} dma_reg_t;

typedef struct {
	u32				   used;
	u32				   channel_count;
	dma_channel_reg_t *channel;
	u32				   reserved;
	dma_desc_t		   *desc;
	dma_irq_handler_t  dma_func;
} dma_source_t;

#define DMA_RST_OFS	   16
#define DMA_GATING_OFS 0

void dma_init(void);
void dma_exit(void);

u32 dma_request(u32 dmatype);
u32 dma_request_from_last(u32 dmatype);
int dma_release(u32 hdma);
int dma_setting(u32 hdma, dma_set_t *cfg);
int dma_start(u32 hdma, u32 saddr, u32 daddr, u32 bytes);
int dma_stop(u32 hdma);
int dma_querystatus(u32 hdma);

int dma_test();

#endif /* _SUNXI_DMA_H */
