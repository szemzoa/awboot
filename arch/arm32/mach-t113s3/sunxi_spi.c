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
#include "sunxi_clk.h"
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

enum {
	OPCODE_READ_ID						= 0x9f,
	OPCODE_READ_STATUS				= 0x0f,
	OPCODE_WRITE_STATUS				= 0x1f,
	OPCODE_READ_PAGE					= 0x13,
	OPCODE_READ								= 0x03,
	OPCODE_FAST_READ					= 0x0b,
	OPCODE_FAST_READ_DUAL_O		= 0x3b,
	OPCODE_FAST_READ_QUAD_O		= 0x6b,
	OPCODE_FAST_READ_DUAL_IO	= 0xbb,
	OPCODE_FAST_READ_QUAD_IO	= 0xeb,
	OPCODE_WRITE_ENABLE				= 0x06,
	OPCODE_BLOCK_ERASE				= 0xd8,
	OPCODE_PROGRAM_LOAD				= 0x02,
	OPCODE_PROGRAM_EXEC				= 0x10,
	OPCODE_RESET							= 0xff,
};

/* Micron calls it "feature", Winbond "status".
   We'll call it config for simplicity. */
enum {
	CONFIG_ADDR_PROTECT		= 0xa0,
	CONFIG_ADDR_OTP				= 0xb0,
	CONFIG_ADDR_STATUS		= 0xc0,
	CONFIG_POS_BUF				= 0x08, // Micron specific
};

enum {
	SPI_GCR_SRST_POS = 31,
	SPI_GCR_SRST_MSK = (1 << SPI_GCR_SRST_POS),
	SPI_GCR_TPEN_POS = 7,
	SPI_GCR_TPEN_MSK = (1 << SPI_GCR_TPEN_POS),
	SPI_GCR_MODE_POS = 1, // Master mode
	SPI_GCR_MODE_MSK = (1 << SPI_GCR_MODE_POS),
	SPI_GCR_EN_POS   = 0,
	SPI_GCR_EN_MSK   = (1 << SPI_GCR_EN_POS),
};

enum {
	SPI_BCC_DUAL_RX = (1 << 28),
	SPI_BCC_QUAD_IO = (1 << 29),
	SPI_BCC_STC_MSK = (0x00ffffff),
	SPI_BCC_DUM_POS = 24,
	SPI_BCC_DUM_MSK = (0xf << SPI_BCC_DUM_POS),
};

enum {
	SPI_MBC_CNT_MSK = (0x00ffffff),
};

enum {
	SPI_MTC_CNT_MSK = (0x00ffffff),
};

enum {
	SPI_TCR_SPOL_POS = 2,
	SPI_TCR_SPOL_MSK = (1 << SPI_TCR_SPOL_POS),
	SPI_TCR_SS_OWNER_POS = 6,
	SPI_TCR_SS_OWNER_MSK = (1 << SPI_TCR_SS_OWNER_POS),
	SPI_TCR_DHB_POS = 8,
	SPI_TCR_DHB_MSK = (1 << SPI_TCR_DHB_POS),
	SPI_TCR_SDC_POS = 11,
	SPI_TCR_SDC_MSK = (1 << SPI_TCR_SDC_POS),
	SPI_TCR_SDM_POS = 13,
	SPI_TCR_SDM_MSK = (1 << SPI_TCR_SDM_POS),
};

enum {
	SPI_FCR_RX_LEVEL_POS = 0,
	SPI_FCR_RX_LEVEL_MSK = (0xff < SPI_FCR_RX_LEVEL_POS),
	SPI_FCR_RX_DRQEN_POS = 8,
	SPI_FCR_RX_DRQEN_MSK = (0x1 << SPI_FCR_RX_DRQEN_POS),
	SPI_FCR_RX_TESTEN_POS = 14,
	SPI_FCR_RX_TESTEN_MSK = (0x1 << SPI_FCR_RX_TESTEN_POS),
	SPI_FCR_RX_RST_POS = 15,
	SPI_FCR_RX_RST_MSK = (0x1 << SPI_FCR_RX_RST_POS),
	SPI_FCR_TX_LEVEL_POS = 16,
	SPI_FCR_TX_LEVEL_MSK = (0xff << SPI_FCR_TX_LEVEL_POS),
	SPI_FCR_TX_DRQEN_POS = 24,
	SPI_FCR_TX_DRQEN_MSK = (0x1 << SPI_FCR_TX_DRQEN_POS),
	SPI_FCR_TX_TESTEN_POS = 30,
	SPI_FCR_TX_TESTEN_MSK = (0x1 << SPI_FCR_TX_TESTEN_POS),
	SPI_FCR_TX_RST_POS = 31,
	SPI_FCR_TX_RST_MSK = (0x1 << SPI_FCR_TX_RST_POS),
};

enum {
	SPI_FSR_RF_CNT_POS	= 0,
	SPI_FSR_RF_CNT_MSK	= (0xff << SPI_FSR_RF_CNT_POS),
	SPI_FSR_TF_CNT_POS	= 16,
	SPI_FSR_TF_CNT_MSK	= (0xff << SPI_FSR_TF_CNT_POS),
};

#define SPINAND_ID(...)			{ .val = { __VA_ARGS__ }, .len = sizeof((uint8_t[]){ __VA_ARGS__ }) }

static const spi_nand_info_t spi_nand_infos[] = {
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

/* SPI Clock Control Register Bit Fields & Masks,default:0x0000_0002 */
#define SPI_CLK_CTL_CDR2        (0xFF <<  0) /* Clock Divide Rate 2,master mode only : SPI_CLK = AHB_CLK/(2*(n+1)) */
#define SPI_CLK_CTL_CDR1        (0xF  <<  8) /* Clock Divide Rate 1,master mode only : SPI_CLK = AHB_CLK/2^n */
#define SPI_CLK_CTL_DRS         (0x1  << 12) /* Divide rate select,default,0:rate 1;1:rate 2 */

#define SPI_MOD_CLK		200000000

static void spi_set_clk(sunxi_spi_t *spi, u32 spi_clk, u32 ahb_clk, u32 cdr)
{
    uint32_t reg_val = 0;
    uint32_t div_clk = 0;

        debug("spi-nand: set spi clock=%dMHz, mclk=%dMHz\r\n", spi_clk / 1000 / 1000, ahb_clk / 1000 / 1000);
        reg_val = read32(spi->base + SPI_CCR);

        /* CDR2 */
        if (cdr) {
                div_clk = ahb_clk / (spi_clk * 2) - 1;
                reg_val &= ~SPI_CLK_CTL_CDR2;
                reg_val |= (div_clk | SPI_CLK_CTL_DRS);
                debug("CDR2 - n = %d\r\n", div_clk);
        } else { /* CDR1 */
                while (ahb_clk > spi_clk) {
                        div_clk++;
                        ahb_clk >>= 1;
                }
                reg_val &= ~(SPI_CLK_CTL_CDR1 | SPI_CLK_CTL_DRS);
                reg_val |= (div_clk << 8);
                debug("CDR1 - n = %d\r\n", div_clk);
        }

        write32(spi->base + SPI_CCR, reg_val);
}

static int spi_clk_init(uint32_t mod_clk)
{
    uint32_t rval;
    uint32_t source_clk = 0;
    uint32_t m, n, divi, factor_m;

        /* SCLK = src/M/N */
        /* N: 00:1 01:2 10:4 11:8 */
        /* M: factor_m + 1 */
        source_clk = sunxi_clk_get_peri1x_rate();

        divi = div((source_clk + mod_clk - 1) , mod_clk);
        divi = divi == 0 ? 1 : divi;
        if (divi > 128) {
                m = 1;
                n = 0;
                return -1;
        } else if (divi > 64) {
                n = 3;
                m = divi >> 3;
        } else if (divi > 32) {
                n = 2;
                m = divi >> 2;
        } else if (divi > 16) {
                n = 1;
                m = divi >> 1;
        } else {
                n = 0;
                m = divi;
        }

        factor_m = m - 1;
        rval = (1U << 31) | (0x1 << 24) | (n << 8) | factor_m;

        /**sclk_freq = source_clk / (1 << n) / m; */
//        debug("spi-nand: parent_clk=%dMHz, div=%d, n=%d, m=%d\r\n", source_clk / 1000 / 1000, divi, n, m);
	write32(T113_CCU_BASE + CCU_SPI0_CLK_REG, rval);

        return 0;
}

#if 0
int sunxi_get_spi_clk()
{
    u32 reg_val = 0;
    u32 src = 0, clk = 0, sclk_freq = 0;
    u32 n, m;

        reg_val = read32(T113_CCU_BASE + CCU_SPI0_CLK_REG);
        src = (reg_val >> 24) & 0x7;
        n = (reg_val >> 8) & 0x3;
        m = ((reg_val >> 0) & 0xf) + 1;

        switch(src) {
                case 0:
                        clk = 24000000;
                        break;
                case 1:
                        clk = sunxi_clk_get_peri1x_rate();
                        break;
                default:
                        clk = 0;
                        break;
        }

        sclk_freq = div(div(clk, (1 << n)), m);
	debug("sclk_freq=%dMHz, reg_val: %x, n=%d, m=%d\r\n", sclk_freq / 1000 / 1000, reg_val, n, m);

        return sclk_freq;
}
#endif

static void spi_reset_fifo(sunxi_spi_t *spi)
{
	uint32_t val = read32(spi->base + SPI_FCR);

	val |= (SPI_FCR_RX_RST_MSK | SPI_FCR_TX_RST_MSK);
	/* Set the trigger level of RxFIFO/TxFIFO. */
	val &= ~(SPI_FCR_RX_LEVEL_MSK | SPI_FCR_TX_LEVEL_MSK);
	val |= (0x20 << SPI_FCR_TX_LEVEL_POS) | (0x20 << SPI_FCR_RX_LEVEL_POS); // IRQ trigger at 32 bytes (half fifo)
	write32(spi->base + SPI_FCR, val);
}

static uint32_t spi_query_txfifo(sunxi_spi_t *spi)
{
	uint32_t val = read32(spi->base + SPI_FSR) & SPI_FSR_TF_CNT_MSK;

	val >>= SPI_FSR_TF_CNT_POS;
	return 0;
}

static uint32_t spi_query_rxfifo(sunxi_spi_t *spi)
{
	uint32_t val = read32(spi->base + SPI_FSR) & SPI_FSR_RF_CNT_MSK;

	val >>= SPI_FSR_RF_CNT_POS;
	return val;
}

int sunxi_spi_init(sunxi_spi_t *spi)
{
	uint32_t val;

	/* Config SPI pins */
	sunxi_gpio_init(spi->gpio_cs.pin, spi->gpio_cs.mux);
	sunxi_gpio_init(spi->gpio_sck.pin, spi->gpio_sck.mux);
	sunxi_gpio_init(spi->gpio_mosi.pin, spi->gpio_mosi.mux);
	sunxi_gpio_init(spi->gpio_miso.pin, spi->gpio_miso.mux);
	sunxi_gpio_init(spi->gpio_wp.pin, spi->gpio_wp.mux);
	sunxi_gpio_init(spi->gpio_hold.pin, spi->gpio_hold.mux);

	// Floating by default
	sunxi_gpio_set_pull(spi->gpio_wp.pin, GPIO_PULL_UP);
	sunxi_gpio_set_pull(spi->gpio_hold.pin, GPIO_PULL_UP);

	/* Deassert spi0 reset */
	val = read32(T113_CCU_BASE + CCU_SPI_BGR_REG);
	val |= (1 << 16);
	write32(T113_CCU_BASE + CCU_SPI_BGR_REG, val);

	/* Open the spi0 gate */
	val = read32(T113_CCU_BASE + CCU_SPI_BGR_REG);
	val |= (1 << 31);
	write32(T113_CCU_BASE + CCU_SPI_BGR_REG, val);

	/* Open the spi0 bus gate */
	val = read32(T113_CCU_BASE + CCU_SPI_BGR_REG);
	val |= (1 << 0);
	write32(T113_CCU_BASE + CCU_SPI_BGR_REG, val);

	spi_clk_init(SPI_MOD_CLK);
	spi_set_clk(spi, spi->clk_rate, SPI_MOD_CLK, 0);

	/* Enable spi0 and do a soft reset */
	val = read32(spi->base + SPI_GCR);
	val |= SPI_GCR_SRST_MSK | SPI_GCR_TPEN_MSK | SPI_GCR_MODE_MSK | SPI_GCR_EN_MSK;
	write32(spi->base + SPI_GCR, val);
	while(read32(spi->base + SPI_GCR) & SPI_GCR_SRST_MSK); // Wait for reset bit to clear

	/* set mode 0, software slave select, discard hash burst, SDC */
	val = read32(spi->base + SPI_TCR);
	val &= ~(0x3 << 0); //  CPOL, CPHA = 0
	val &= ~(SPI_TCR_SDM_MSK | SPI_TCR_SDC_MSK);
	val |= SPI_TCR_SPOL_MSK | SPI_TCR_DHB_MSK;
	val |= SPI_TCR_SDC_MSK; // Set SDC since we are above 60MHz
	write32(spi->base + SPI_TCR, val);

	spi_reset_fifo(spi);

	return 0;
}

void sunxi_spi_disable(sunxi_spi_t *spi)
{
	uint32_t val;

	/* Disable the spi0 controller */
	val = read32(spi->base + SPI_GCR);
	val &= ~((1 << 1) | (1 << 0));
	write32(spi->base + SPI_GCR, val);
}

/*
*	txlen: transmit length
* rxlen: receive length
* stxlen: single transmit length (for sending opcode + address/param as single tx when quad mode is on)
* dummylen: dummy bytes length
*/
static void spi_set_counters(sunxi_spi_t *spi, int txlen, int rxlen, int stxlen, int dummylen)
{
	uint32_t val;

	val = read32(spi->base + SPI_MBC);
	val &= ~SPI_MBC_CNT_MSK;
	val |= (SPI_MBC_CNT_MSK & (txlen + rxlen + dummylen));
	write32(spi->base + SPI_MBC, val);

	val = read32(spi->base + SPI_MTC);
	val &= ~SPI_MTC_CNT_MSK;
	val |= (SPI_MTC_CNT_MSK & txlen);
	write32(spi->base + SPI_MTC, val);

	val = read32(spi->base + SPI_BCC);
	val &= ~SPI_BCC_STC_MSK;
	val |= (SPI_BCC_STC_MSK & stxlen);
	val &= ~SPI_BCC_DUM_MSK;
	val |= (dummylen << SPI_BCC_DUM_POS);
	write32(spi->base + SPI_BCC, val);
}

static void spi_write_tx_fifo(sunxi_spi_t *spi, uint8_t *buf, int len)
{
	while (len-- > 0) {
		while (spi_query_txfifo(spi) >= 64) {};
		write8(spi->base + SPI_TXD, *buf++);
	}
}

static uint32_t spi_read_rx_fifo(sunxi_spi_t *spi, uint8_t *buf, int len)
{
	// uint32_t fifo_len = spi_query_rxfifo(spi);

	// // Do a quick read with what's already in FIFO
	// len -= fifo_len;
	// while (fifo_len-- > 0) {
	// 	*buf++ = read8(spi->base + SPI_RXD);
	// }

	// Wait for additional data
	while (len-- > 0) {
		while (spi_query_rxfifo(spi) < 1) {};
		*buf++ = read8(spi->base + SPI_RXD);
	}
	return len;
}

static void spi_set_io_mode(sunxi_spi_t *spi, spi_io_mode_t mode)
{
	uint32_t bcc;
	bcc = read32(spi->base + SPI_BCC);
	bcc &= ~(SPI_BCC_QUAD_IO | SPI_BCC_DUAL_RX);
	switch (mode)
	{
	case SPI_IO_MODE_DUAL_RX:
		bcc |= SPI_BCC_DUAL_RX;
		break;
	case SPI_IO_MODE_QUAD_IO:
		bcc |= SPI_BCC_QUAD_IO;
		break;
	case SPI_IO_MODE_SINGLE:
	default:
		break;
	}
	write32(spi->base + SPI_BCC, bcc);
}

static int spi_transfer(sunxi_spi_t *spi, spi_io_mode_t mode, void *txbuf, int txlen, void *rxbuf, int rxlen)
{
	int stxlen;

	spi_set_io_mode(spi, mode);

	switch (mode)
	{
	case SPI_IO_MODE_QUAD_IO:
		stxlen = 1; // Only opcode
		break;
	case SPI_IO_MODE_DUAL_RX:
	case SPI_IO_MODE_QUAD_RX:
		stxlen = txlen; // Only tx data
		break;
	case SPI_IO_MODE_SINGLE:
	default:
		stxlen = txlen + rxlen; // both tx and rx data
		break;
	};

	// Full size of transfer, controller will wait for TX FIFO to be filled if txlen > 0
	spi_set_counters(spi, txlen, rxlen, stxlen, 0);
	spi_reset_fifo(spi);

	write32(spi->base + SPI_TCR, read32(spi->base + SPI_TCR) | (1 << 31)); // Start exchange when data in FIFO

	if(txbuf && txlen > 0) {
		spi_write_tx_fifo(spi, txbuf, txlen);
	}

	if(rxbuf && rxlen > 0) {
		spi_read_rx_fifo(spi, rxbuf, rxlen);
	}

	return txlen + rxlen;
}
/*
* SPI NAND functions
*/

static int spi_nand_info(sunxi_spi_t *spi)
{
	spi_nand_info_t *t;
	uint8_t tx[2];
	uint8_t rx[4];
	int i, r;

	tx[0] = OPCODE_READ_ID;
	tx[1] = 0x0;
	r = spi_transfer(spi, SPI_IO_MODE_SINGLE, tx, 2, rx, 4);
	if(r < 0)
		return r;

	for(i = 0; i < ARRAY_SIZE(spi_nand_infos); i++)
	{
		t = (spi_nand_info_t *)&spi_nand_infos[i];
		if(memcmp(rx, t->id.val, t->id.len) == 0)
		{
			memcpy((void *)&spi->info, t, sizeof(spi_nand_info_t));
			return 0;
		}
	}

	tx[0] = OPCODE_READ_ID;
	r = spi_transfer(spi, SPI_IO_MODE_SINGLE, tx, 1, rx, 4);
	if(r < 0)
		return r;

	for(i = 0; i < ARRAY_SIZE(spi_nand_infos); i++)
	{
		t = (spi_nand_info_t *)&spi_nand_infos[i];
		if(memcmp(rx, t->id.val, t->id.len) == 0)
		{
			memcpy((void *)&spi->info, t, sizeof(spi_nand_info_t));
			return 0;
		}
	}

	debug("spi-nand id '0x%02x%02x%02x%02x' is unknown\r\n", rx[0], rx[1], rx[2], rx[3]);

	return -1;
}

static int spi_nand_reset(sunxi_spi_t *spi)
{
	uint8_t tx[1];
	int r;

	tx[0] = OPCODE_RESET;
	r = spi_transfer(spi, SPI_IO_MODE_SINGLE, tx, 1, 0, 0);
	if(r < 0)
		return -1;

	udelay(100 * 1000);

	return 0;
}

static int spi_nand_get_config(sunxi_spi_t *spi, uint8_t addr, uint8_t * val)
{
	uint8_t tx[2];
	int r;

	tx[0] = OPCODE_READ_STATUS;
	tx[1] = addr;
	r = spi_transfer(spi, SPI_IO_MODE_SINGLE, tx, 2, val, 1);
	if(r < 0)
		return -1;

	return 0;
}

static int spi_nand_set_config(sunxi_spi_t *spi, uint8_t addr, uint8_t val)
{
	uint8_t tx[3];
	int r;

	tx[0] = OPCODE_WRITE_STATUS;
	tx[1] = addr;
	tx[2] = val;
	r = spi_transfer(spi, SPI_IO_MODE_SINGLE, tx, 3, 0, 0);
	if(r < 0)
		return -1;

	return 0;
}

static void spi_nand_wait_while_busy(sunxi_spi_t *spi)
{
	uint8_t tx[2];
	uint8_t rx[1];
	int r;

	tx[0] = OPCODE_READ_STATUS;
	tx[1] = 0xc0; // SR3
	rx[0] = 0x00;

	do {
		r = spi_transfer(spi, SPI_IO_MODE_SINGLE, tx, 2, rx, 1);
		if(r < 0)
			break;
	} while((rx[0] & 0x1) == 0x1); // SR3 Busy bit
}

int spi_nand_detect(sunxi_spi_t *spi)
{
	uint8_t val;

	spi_nand_reset(spi);
	spi_nand_wait_while_busy(spi);

	if (spi_nand_info(spi) == 0) {

		if ((spi_nand_get_config(spi, CONFIG_ADDR_PROTECT, &val) == 0) && (val != 0x0)) {
			spi_nand_set_config(spi, CONFIG_ADDR_PROTECT, 0x0);
			spi_nand_wait_while_busy(spi);
		}

		// if (strncmp("W25N", spi->info.name, 4) == 0) {
		// 	if ((spi_nand_get_config(spi, CONFIG_ADDR_OTP, &val) == 0) && (val != 0x0)) {
		// 		val &= ~CONFIG_POS_BUF;
		// 		spi_nand_set_config(spi, CONFIG_ADDR_OTP, val);
		// 		spi_nand_wait_for_busy(spi);
		// 	}
		// }

		debug("spi-nand: %s detected\r\n", spi->info.name);

		return 0;
	}

	debug("spi-nand: flash not found\r\n");
	return -1;
}

int spi_nand_load_page(sunxi_spi_t *spi, uint64_t offset)
{
	uint32_t pa;
	uint8_t tx[4];
	int r;

	pa = offset / spi->info.page_size;

	tx[0] = OPCODE_READ_PAGE;
	tx[1] = (uint8_t)(pa >> 16);
	tx[2] = (uint8_t)(pa >> 8);
	tx[3] = (uint8_t)(pa >> 0);

	spi_transfer(spi, SPI_IO_MODE_SINGLE, tx, 4, 0, 0);
	spi_nand_wait_while_busy(spi);

	return 0;
}

uint64_t spi_nand_read(sunxi_spi_t *spi, spi_io_mode_t mode, uint8_t *buf, uint64_t offset, uint64_t count)
{
	uint32_t addr = offset;
	uint32_t cnt = count;
	uint32_t len = 0;
	uint32_t ca;
	uint32_t n;
	uint8_t tx[4];
	int r;

	int read_opcode = OPCODE_READ;
	switch (mode)
	{
	case SPI_IO_MODE_SINGLE:
		read_opcode = OPCODE_READ;
		break;
	case SPI_IO_MODE_DUAL_RX:
		read_opcode = OPCODE_FAST_READ_DUAL_O;
		break;
	case SPI_IO_MODE_DUAL_IO:
		read_opcode = OPCODE_FAST_READ_DUAL_IO;
		break;
	case SPI_IO_MODE_QUAD_RX:
		read_opcode = OPCODE_FAST_READ_QUAD_O;
		break;
	case SPI_IO_MODE_QUAD_IO:
		read_opcode = OPCODE_FAST_READ_QUAD_IO;
		break;

	default:
		break;
	};

	while(cnt > 0) {

		ca = addr & (spi->info.page_size - 1);
		n = cnt > (spi->info.page_size - ca) ? (spi->info.page_size - ca) : cnt;

		spi_nand_load_page(spi, addr);

		tx[0] = read_opcode;
		tx[1] = (uint8_t)(ca >> 8);
		tx[2] = (uint8_t)(ca >> 0);
		tx[3] = 0x0;

		spi_transfer(spi, mode, tx, 4, buf, n);

		addr += n;
		buf += n;
		len += n;
		cnt -= n;
	}

	return len;
}
