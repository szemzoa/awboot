/*
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

#include "div.h"
#include "reg-ccu.h"
#include "sunxi_spi.h"
#include "debug.h"

enum {
	SPI_GCR	= 0x04,
	SPI_TCR	= 0x08,
	SPI_IER	= 0x10,
	SPI_ISR	= 0x14,
	SPI_FCR	= 0x18,
	SPI_FSR	= 0x1c,
	SPI_WCR	= 0x20,
	SPI_CCR	= 0x24,
	SPI_MBC	= 0x30,
	SPI_MTC	= 0x34,
	SPI_BCC	= 0x38,
	SPI_TXD	= 0x200,
	SPI_RXD	= 0x300,
};

#define SPI0_CLK_DIV_NONE	0x0000
#define SPI0_CLK_DIV_BY_2       0x1000
#define SPI0_CLK_DIV_BY_4       0x1001

enum {
	OPCODE_RDID			= 0x9f,
	OPCODE_GET_FEATURE		= 0x0f,
	OPCODE_SET_FEATURE		= 0x1f,
	OPCODE_FEATURE_PROTECT		= 0xa0,
	OPCODE_FEATURE_CONFIG		= 0xb0,
	OPCODE_FEATURE_STATUS		= 0xc0,
	OPCODE_READ_PAGE_TO_CACHE	= 0x13,
	OPCODE_READ_PAGE_FROM_CACHE	= 0x03,
	OPCODE_WRITE_ENABLE		= 0x06,
	OPCODE_BLOCK_ERASE		= 0xd8,
	OPCODE_PROGRAM_LOAD		= 0x02,
	OPCODE_PROGRAM_EXEC		= 0x10,
	OPCODE_RESET			= 0xff,
};

/*
#define SPINAND_PAGE_BITS	(11)
#define SPINAND_PAGE_MASK	((1 << SPINAND_PAGE_BITS) - 1)
#define SPINAND_PAGE_SIZE	(1 << SPINAND_PAGE_BITS)
*/

#define SPINAND_ID(...)			{ .val = { __VA_ARGS__ }, .len = sizeof((uint8_t[]){ __VA_ARGS__ }) }

static const struct spinand_info_t spinand_infos[] = {
	/* Winbond */
	{ "W25N512GV",       SPINAND_ID(0xef, 0xaa, 0x20), 2048,  64,  64,  512, 1, 1 },
	{ "W25N01GV",        SPINAND_ID(0xef, 0xaa, 0x21), 2048,  64,  64, 1024, 1, 1 },
	{ "W25M02GV",        SPINAND_ID(0xef, 0xab, 0x21), 2048,  64,  64, 1024, 1, 2 },
	{ "W25N02KV",        SPINAND_ID(0xef, 0xaa, 0x22), 2048, 128,  64, 2048, 1, 1 },

	/* Gigadevice */
	{ "GD5F1GQ4UAWxx",   SPINAND_ID(0xc8, 0x10),       2048,  64,  64, 1024, 1, 1 },
	{ "GD5F1GQ5UExxG",   SPINAND_ID(0xc8, 0x51),       2048, 128,  64, 1024, 1, 1 },
	{ "GD5F1GQ4UExIG",   SPINAND_ID(0xc8, 0xd1),       2048, 128,  64, 1024, 1, 1 },
	{ "GD5F1GQ4UExxH",   SPINAND_ID(0xc8, 0xd9),       2048,  64,  64, 1024, 1, 1 },
	{ "GD5F1GQ4xAYIG",   SPINAND_ID(0xc8, 0xf1),       2048,  64,  64, 1024, 1, 1 },
	{ "GD5F2GQ4UExIG",   SPINAND_ID(0xc8, 0xd2),       2048, 128,  64, 2048, 1, 1 },
	{ "GD5F2GQ5UExxH",   SPINAND_ID(0xc8, 0x32),       2048,  64,  64, 2048, 1, 1 },
	{ "GD5F2GQ4xAYIG",   SPINAND_ID(0xc8, 0xf2),       2048,  64,  64, 2048, 1, 1 },
	{ "GD5F4GQ4UBxIG",   SPINAND_ID(0xc8, 0xd4),       4096, 256,  64, 2048, 1, 1 },
	{ "GD5F4GQ4xAYIG",   SPINAND_ID(0xc8, 0xf4),       2048,  64,  64, 4096, 1, 1 },
	{ "GD5F2GQ5UExxG",   SPINAND_ID(0xc8, 0x52),       2048, 128,  64, 2048, 1, 1 },
	{ "GD5F4GQ4UCxIG",   SPINAND_ID(0xc8, 0xb4),       4096, 256,  64, 2048, 1, 1 },
	{ "GD5F4GQ4RCxIG",   SPINAND_ID(0xc8, 0xa4),       4096, 256,  64, 2048, 1, 1 },

	/* Macronix */
	{ "MX35LF1GE4AB",    SPINAND_ID(0xc2, 0x12),       2048,  64,  64, 1024, 1, 1 },
	{ "MX35LF1G24AD",    SPINAND_ID(0xc2, 0x14),       2048, 128,  64, 1024, 1, 1 },
	{ "MX31LF1GE4BC",    SPINAND_ID(0xc2, 0x1e),       2048,  64,  64, 1024, 1, 1 },
	{ "MX35LF2GE4AB",    SPINAND_ID(0xc2, 0x22),       2048,  64,  64, 2048, 1, 1 },
	{ "MX35LF2G24AD",    SPINAND_ID(0xc2, 0x24),       2048, 128,  64, 2048, 1, 1 },
	{ "MX35LF2GE4AD",    SPINAND_ID(0xc2, 0x26),       2048, 128,  64, 2048, 1, 1 },
	{ "MX35LF2G14AC",    SPINAND_ID(0xc2, 0x20),       2048,  64,  64, 2048, 1, 1 },
	{ "MX35LF4G24AD",    SPINAND_ID(0xc2, 0x35),       4096, 256,  64, 2048, 1, 1 },
	{ "MX35LF4GE4AD",    SPINAND_ID(0xc2, 0x37),       4096, 256,  64, 2048, 1, 1 },

	/* Micron */
	{ "MT29F1G01AAADD",  SPINAND_ID(0x2c, 0x12),       2048,  64,  64, 1024, 1, 1 },
	{ "MT29F1G01ABAFD",  SPINAND_ID(0x2c, 0x14),       2048, 128,  64, 1024, 1, 1 },
	{ "MT29F2G01AAAED",  SPINAND_ID(0x2c, 0x9f),       2048,  64,  64, 2048, 2, 1 },
	{ "MT29F2G01ABAGD",  SPINAND_ID(0x2c, 0x24),       2048, 128,  64, 2048, 2, 1 },
	{ "MT29F4G01AAADD",  SPINAND_ID(0x2c, 0x32),       2048,  64,  64, 4096, 2, 1 },
	{ "MT29F4G01ABAFD",  SPINAND_ID(0x2c, 0x34),       4096, 256,  64, 2048, 1, 1 },
	{ "MT29F4G01ADAGD",  SPINAND_ID(0x2c, 0x36),       2048, 128,  64, 2048, 2, 2 },
	{ "MT29F8G01ADAFD",  SPINAND_ID(0x2c, 0x46),       4096, 256,  64, 2048, 1, 2 },
};

void sunxi_spi_init(struct sunxi_spi_t *spi)
{
    uint32_t addr, val;

        /* Config SPI pins */
        sunxi_gpio_init(spi->gpio_cs.pin, spi->gpio_cs.mux);
        sunxi_gpio_init(spi->gpio_sck.pin, spi->gpio_sck.mux);
        sunxi_gpio_init(spi->gpio_mosi.pin, spi->gpio_mosi.mux);
        sunxi_gpio_init(spi->gpio_miso.pin, spi->gpio_miso.mux);

	/* Deassert spi0 reset */
	addr = T113_CCU_BASE + CCU_SPI_BGR_REG;
	val = read32(addr);
	val |= (1 << 16);
	write32(addr, val);

	/* Open the spi0 gate */
	addr = T113_CCU_BASE + CCU_SPI0_CLK_REG;
	val = read32(addr);
	val |= (1 << 31);
	write32(addr, val);

	/* Open the spi0 bus gate */
	addr = T113_CCU_BASE + CCU_SPI_BGR_REG;
	val = read32(addr);
	val |= (1 << 0);
	write32(addr, val);

	/* Select 24MHz for spi0 clk */
	addr = T113_CCU_BASE + CCU_SPI0_CLK_REG;
	val = read32(addr);
	val &= ~(0x3 << 24);
	write32(addr, val);

	/* Set spi clock rate control register */
	write32(spi->base + SPI_CCR, SPI0_CLK_DIV_NONE);

	/* Enable spi0 and do a soft reset */
	val = read32(spi->base + SPI_GCR);
	val |= (1 << 31) | (1 << 7) | (1 << 1) | (1 << 0);
	write32(spi->base + SPI_GCR, val);
	while(read32(spi->base + SPI_GCR) & (1 << 31));

	/* set mode 0 */
	val = read32(spi->base + SPI_TCR);
	val &= ~(0x3 << 0);
	val |= (1 << 6) | (1 << 2);
	write32(spi->base + SPI_TCR, val);

	/* reset FIFOs */
	val = read32(spi->base + SPI_FCR);
	val |= (1 << 31) | (1 << 15);
	write32(spi->base + SPI_FCR, val);
}

void sunxi_spi_exit(struct sunxi_spi_t *spi)
{
    uint32_t val;

	/* Disable the spi0 controller */
	val = read32(spi->base + SPI_GCR);
	val &= ~((1 << 1) | (1 << 0));
	write32(spi->base + SPI_GCR, val);
}

static void spi_select(struct sunxi_spi_t *spi)
{
    uint32_t val;

	val = read32(spi->base + SPI_TCR);
	val &= ~((0x3 << 4) | (0x1 << 7));
	val |= ((0 & 0x3) << 4) | (0x0 << 7);
	write32(spi->base + SPI_TCR, val);
}

static void spi_deselect(struct sunxi_spi_t *spi)
{
    uint32_t val;

	val = read32(spi->base + SPI_TCR);
	val &= ~((0x3 << 4) | (0x1 << 7));
	val |= ((0 & 0x3) << 4) | (0x1 << 7);
	write32(spi->base + SPI_TCR, val);
}

static void spi_write_txbuf(struct sunxi_spi_t *spi, uint8_t *buf, int len)
{
    int i;

	write32(spi->base + SPI_MTC, len & 0xffffff);
	write32(spi->base + SPI_BCC, len & 0xffffff);

	if(buf) {
	    for(i = 0; i < len; i++)
	    	write8(spi->base + SPI_TXD, *buf++);

	} else {
	    for(i = 0; i < len; i++)
		write8(spi->base + SPI_TXD, 0xff);
	}
}

static int spi_transfer(struct sunxi_spi_t *spi, void *txbuf, void *rxbuf, int len)
{
    int count = len;
    uint8_t *tx = txbuf;
    uint8_t *rx = rxbuf;
    uint8_t val;
    int n, i;

	while (count > 0) {
		n = (count <= 64) ? count : 64;
		write32(spi->base + SPI_MBC, n);
		spi_write_txbuf(spi, tx, n);
		write32(spi->base + SPI_TCR, read32(spi->base + SPI_TCR) | (1 << 31));

		while((read32(spi->base + SPI_FSR) & 0xff) < n)
		    ;
		for(i = 0; i < n; i++) {
		    val = read8(spi->base + SPI_RXD);
		    if(rx)
			*rx++ = val;
		}

		if(tx)
		    tx += n;

		count -= n;
	}

	return len;
}

static int spi_write_then_read(struct sunxi_spi_t *spi, void *txbuf, int txlen, void *rxbuf, int rxlen)
{
	if (spi_transfer(spi, txbuf, NULL, txlen) != txlen)
	    return -1;

	if (spi_transfer(spi, NULL, rxbuf, rxlen) != rxlen)
	    return -1;

	return 0;
}

static int spinand_info(struct sunxi_spi_t *spi)
{
    struct spinand_info_t *t;
    uint8_t tx[2];
    uint8_t rx[4];
    int i, r;

	tx[0] = OPCODE_RDID;
	tx[1] = 0x0;
	spi_select(spi);
	r = spi_write_then_read(spi, tx, 2, rx, 4);
	spi_deselect(spi);
	if(r < 0)
		return -1;

	for(i = 0; i < ARRAY_SIZE(spinand_infos); i++)
	{
		t = (struct spinand_info_t *)&spinand_infos[i];
		if(memcmp(rx, t->id.val, t->id.len) == 0)
		{
			memcpy((void *)&spi->info, t, sizeof(struct spinand_info_t));
			return 0;
		}
	}

	tx[0] = OPCODE_RDID;
	spi_select(spi);
	r = spi_write_then_read(spi, tx, 1, rx, 4);
	spi_deselect(spi);
	if(r < 0)
		return -1;

	for(i = 0; i < ARRAY_SIZE(spinand_infos); i++)
	{
		t = (struct spinand_info_t *)&spinand_infos[i];
		if(memcmp(rx, t->id.val, t->id.len) == 0)
		{
			memcpy((void *)&spi->info, t, sizeof(struct spinand_info_t));
			return 0;
		}
	}

	debug("spi-nand id '0x%02x%02x%02x%02x' is unknown\r\n", rx[0], rx[1], rx[2], rx[3]);

	return -1;
}

static int spinand_reset(struct sunxi_spi_t *spi)
{
    uint8_t tx[1];
    int r;

	tx[0] = OPCODE_RESET;
	spi_select(spi);
	r = spi_write_then_read(spi, tx, 1, 0, 0);
	spi_deselect(spi);
	if(r < 0)
		return -1;

	udelay(100 * 1000);

	return 0;
}

static int spinand_get_feature(struct sunxi_spi_t *spi, uint8_t addr, uint8_t * val)
{
    uint8_t tx[2];
    int r;

	tx[0] = OPCODE_GET_FEATURE;
	tx[1] = addr;
	spi_select(spi);
	r = spi_write_then_read(spi, tx, 2, val, 1);
	spi_deselect(spi);
	if(r < 0)
		return -1;

	return 0;
}

static int spinand_set_feature(struct sunxi_spi_t *spi, uint8_t addr, uint8_t val)
{
    uint8_t tx[3];
    int r;

	tx[0] = OPCODE_SET_FEATURE;
	tx[1] = addr;
	tx[2] = val;
	spi_select(spi);
	r = spi_write_then_read(spi, tx, 3, 0, 0);
	spi_deselect(spi);
	if(r < 0)
		return -1;

	return 0;
}

static void spinand_wait_for_busy(struct sunxi_spi_t *spi)
{
    uint8_t tx[2];
    uint8_t rx[1];
    int r;

	tx[0] = OPCODE_GET_FEATURE;
	tx[1] = 0xc0;
	rx[0] = 0x00;

	do {
	    spi_select(spi);
	    r = spi_write_then_read(spi, tx, 2, rx, 1);
	    spi_deselect(spi);
	    if(r < 0)
	    	break;
	} while((rx[0] & 0x1) == 0x1);
}

int spinand_detect(struct sunxi_spi_t *spi)
{
    uint8_t val;

	spinand_reset(spi);

	if (spinand_info(spi) == 0) {
//	    spinand_reset(spi);
	    spinand_wait_for_busy(spi);

	    if ((spinand_get_feature(spi, OPCODE_FEATURE_PROTECT, &val) == 0) && (val != 0x0)) {
	    	spinand_set_feature(spi, OPCODE_FEATURE_PROTECT, 0x0);
	    	spinand_wait_for_busy(spi);
	    }

	    debug("spi-nand: %s detected\r\n", spi->info.name);

	    return 0;
	}

	debug("spi-nand: flash not found\r\n");
	return -1;
}

uint64_t spinand_read(struct sunxi_spi_t *spi, uint8_t *buf, uint64_t offset, uint64_t count)
{
    uint32_t addr = offset;
    uint32_t cnt = count;
    uint32_t len = 0;
    uint32_t pa, ca;
    uint32_t n;
    uint8_t tx[4];
    int r;

	while(cnt > 0) {

//		pa = addr / spi->info.page_size;
		pa = div(addr, spi->info.page_size);

		ca = addr & (spi->info.page_size - 1);
		n = cnt > (spi->info.page_size - ca) ? (spi->info.page_size - ca) : cnt;

		tx[0] = OPCODE_READ_PAGE_TO_CACHE;
		tx[1] = (uint8_t)(pa >> 16);
		tx[2] = (uint8_t)(pa >> 8);
		tx[3] = (uint8_t)(pa >> 0);

		spi_select(spi);
		r = spi_write_then_read(spi, tx, 4, 0, 0);
		spi_deselect(spi);
		if(r < 0)
			break;

		spinand_wait_for_busy(spi);

		tx[0] = OPCODE_READ_PAGE_FROM_CACHE;
		tx[1] = (uint8_t)(ca >> 8);
		tx[2] = (uint8_t)(ca >> 0);
		tx[3] = 0x0;

		spi_select(spi);
		r = spi_write_then_read(spi, tx, 4, buf, n);
		spi_deselect(spi);
		if(r < 0)
			break;

		spinand_wait_for_busy(spi);

		addr += n;
		buf += n;
		len += n;
		cnt -= n;
	}

	return len;
}
