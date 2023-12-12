#include "main.h"
#include "board.h"
#include "sunxi_clk.h"
#include "reg-ccu.h"
#include "sunxi_ccu.h"
#include "debug.h"

//volatile ccu_reg_t *const ccu = (ccu_reg_t *)T113_CCU_BASE;

void set_pll_cpux_axi(void)
{
	uint32_t val;

	/* AXI: Select cpu clock src to PLL_PERI(1x) */
	write32(CCU_BASE + CCU_CPU_AXI_CFG_REG, (4 << 24) | (1 << 0));
	sdelay(10);

	/* Disable pll gating */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val &= ~(1 << 27);
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Enable pll ldo */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val |= (1 << 30);
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);
	sdelay(5);

	/* Set clk to 1008mhz (default) or CONFIG_CPU_FREQ */
	/* PLL_CPUX = 24 MHz*N/P */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val &= ~((0x3 << 16) | (0xff << 8) | (0x3 << 0));
#ifdef CONFIG_CPU_FREQ
	val |= (((CONFIG_CPU_FREQ / 24000000) - 1) << 8);
#else
	val |= (41 << 8);
#endif
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Lock enable */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val |= (1 << 29);
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Enable pll */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val |= (1 << 31);
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Wait pll stable */
	while (!(read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG) & (0x1 << 28)))
		;
	sdelay(20);

	/* Enable pll gating */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val |= (1 << 27);
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Lock disable */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val &= ~(1 << 29);
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);
	sdelay(1);

	/* AXI: set and change cpu clk src to PLL_CPUX, PLL_CPUX:AXI0 = 1200MHz:600MHz */
	val = read32(CCU_BASE + CCU_CPU_AXI_CFG_REG);
	val &= ~(0x07 << 24 | 0x3 << 16 | 0x3 << 8 | 0xf << 0); // Clear
	val |= (0x03 << 24 | 0x0 << 16 | 0x1 << 8 | 0x1 << 0); // CLK_SEL=PLL_CPU/P, DIVP=0, DIV2=1, DIV1=1
	write32(CCU_BASE + CCU_CPU_AXI_CFG_REG, val);
	sdelay(1);
}

static void set_pll_periph0(void)
{
	uint32_t val;

	/* Periph0 has been enabled */
	if (read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG) & (1 << 31))
		return;

	/* Set default val */
	write32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG, 0x63 << 8);

	/* Lock enable */
	val = read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	val |= (1 << 29);
	write32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG, val);

	/* Enabe pll 600m(1x) 1200m(2x) */
	val = read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	val |= (1 << 31);
	write32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG, val);

	/* Wait pll stable */
	while (!(read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG) & (0x1 << 28)))
		;
	sdelay(20);

	/* Lock disable */
	val = read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	val &= ~(1 << 29);
	write32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG, val);
}

static void set_ahb(void)
{
	write32(CCU_BASE + CCU_PSI_CLK_REG, (2 << 0) | (0 << 8) | (0x03 << 24));
	sdelay(1);
}

static void set_apb(void)
{
	write32(CCU_BASE + CCU_APB0_CLK_REG, (2 << 0) | (1 << 8) | (0x03 << 24));
	sdelay(1);
}

static void set_dma(void)
{
	/* Dma reset */
	write32(CCU_BASE + CCU_DMA_BGR_REG, read32(CCU_BASE + CCU_DMA_BGR_REG) | (1 << 16));
	sdelay(20);
	/* Enable gating clock for dma */
	write32(CCU_BASE + CCU_DMA_BGR_REG, read32(CCU_BASE + CCU_DMA_BGR_REG) | (1 << 0));
}

static void set_mbus(void)
{
	uint32_t val;

	/* Reset mbus domain */
	val = read32(CCU_BASE + CCU_MBUS_CLK_REG);
	val |= (0x1 << 30);
	write32(CCU_BASE + CCU_MBUS_CLK_REG, val);
	sdelay(1);

	/* Enable mbus master clock gating */
	write32(CCU_BASE + CCU_MBUS_MAT_CLK_GATING_REG, 0x00000d87);
}

static void set_module(virtual_addr_t addr)
{
	uint32_t val;

	if (!(read32(addr) & (1 << 31))) {
		val = read32(addr);
		write32(addr, val | (1 << 31) | (1 << 30));

		/* Lock enable */
		val = read32(addr);
		val |= (1 << 29);
		write32(addr, val);

		/* Wait pll stable */
		while (!(read32(addr) & (0x1 << 28)))
			;
		sdelay(20);

		/* Lock disable */
		val = read32(addr);
		val &= ~(1 << 29);
		write32(addr, val);
	}
}

void sunxi_clk_init(void)
{
	set_pll_cpux_axi();
	set_pll_periph0();
	set_ahb();
	set_apb();
	set_dma();
	set_mbus();
	set_module(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	set_module(CCU_BASE + CCU_PLL_VIDEO0_CTRL_REG);
	set_module(CCU_BASE + CCU_PLL_VIDEO1_CTRL_REG);
	set_module(CCU_BASE + CCU_PLL_VE_CTRL);
	set_module(CCU_BASE + CCU_PLL_AUDIO0_CTRL_REG);
	set_module(CCU_BASE + CCU_PLL_AUDIO1_CTRL_REG);
}

uint32_t sunxi_clk_get_peri1x_rate()
{
	uint32_t reg32;
	uint8_t	 plln, pllm, p0;

	/* PLL PERIx */
	reg32 = read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0	 = ((reg32 >> 16) & 0x03) + 1;

		return ((((24 * plln) / (pllm * p0)) >> 1) * 1000 * 1000);
	}

	return 0;
}

int spi_clk_init(sunxi_spi_t *spi)
{
        uint32_t rval;
        uint32_t source_clk = 0;
        uint32_t m, n, divi, factor_m;

        /* SCLK = src/M/N */
        /* N: 00:1 01:2 10:4 11:8 */
        /* M: factor_m + 1 */
        source_clk = sunxi_clk_get_peri1x_rate();

        divi = (source_clk + spi->mod_clk - 1) / spi->mod_clk;
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
        rval     = (1U << 31) | (0x1 << 24) | (n << 8) | factor_m;
        trace("SPI: parent_clk=%" PRIu32 "MHz, div=%" PRIu32 ", n=%" PRIu32 ", m=%" PRIu32 "\r\n", source_clk / 1000 / 1000, divi, n + 1, m);
        write32(CCU_BASE + CCU_SPI0_CLK_REG, rval);

        return 0;
}

#ifdef CONFIG_ENABLE_CPU_FREQ_DUMP
void sunxi_clk_dump()
{
	uint32_t	reg32;
	uint32_t	cpu_clk_src;
	uint32_t UNUSED_DEBUG	plln, pllm;
	uint8_t		p0;
	uint8_t	UNUSED_DEBUG	p1;
	const char * UNUSED_DEBUG clock_str;

	/* PLL CPU */
	reg32		= read32(CCU_BASE + CCU_CPU_AXI_CFG_REG);
	cpu_clk_src = (reg32 >> 24) & 0x7;

	switch (cpu_clk_src) {
		case 0x0:
			clock_str = "OSC24M";
			break;

		case 0x1:
			clock_str = "CLK32";
			break;

		case 0x2:
			clock_str = "CLK16M_RC";
			break;

		case 0x3:
			clock_str = "PLL_CPU";
			break;

		case 0x4:
			clock_str = "PLL_PERI(1X)";
			break;

		case 0x5:
			clock_str = "PLL_PERI(2X)";
			break;

		case 0x6:
			clock_str = "PLL_PERI(800M)";
			break;

		default:
			clock_str = "ERROR";
	}

	reg32 = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	p0	  = (reg32 >> 16) & 0x03;
	if (p0 == 0) {
		p1 = 1;
	} else if (p0 == 1) {
		p1 = 2;
	} else if (p0 == 2) {
		p1 = 4;
	} else {
		p1 = 1;
	}

	debug("CLK: CPU PLL=%s FREQ=%" PRIu32 "MHz\r\n", clock_str, ((((reg32 >> 8) & 0xff) + 1) * 24 / p1));

	/* PLL PERIx */
	reg32 = read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0	 = ((reg32 >> 16) & 0x03) + 1;
		p1	 = ((reg32 >> 20) & 0x03) + 1;

		debug("CLK: PLL_peri (2X)=%" PRIu32 "MHz, (1X)=%" PRIu32 "MHz, (800M)=%" PRIu32 "MHz\r\n", (24 * plln) / (pllm * p0),
			  (24 * plln) / (pllm * p0) >> 1, (24 * plln) / (pllm * p1));
	} else {
		debug("CLK: PLL_peri disabled\r\n");
	}

	/* PLL DDR */
	reg32 = read32(CCU_BASE + CCU_PLL_DDR_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;

		pllm = (reg32 & 0x01) + 1;
		p1	 = ((reg32 >> 1) & 0x1) + 1;
		p0	 = (reg32 & 0x01) + 1;

		debug("CLK: PLL_ddr=%" PRIu32 "MHz\r\n", (24 * plln) / (p0 * p1));

	} else {
		debug("CLK: PLL_ddr disabled\r\n");
	}
}
#endif
