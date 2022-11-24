#include "main.h"
#include "board.h"
#include "div.h"
#include "sunxi_clk.h"
#include "reg-ccu.h"
#include "debug.h"

void set_pll_cpux_axi(void)
{
	uint32_t val;

	/* Select cpux clock src to osc24m, axi divide ratio is 3, system apb clk ratio is 4 */
	write32(T113_CCU_BASE + CCU_CPU_AXI_CFG_REG, (0 << 24) | (3 << 8) | (1 << 0));
	sdelay(1);

	/* Disable pll gating */
	val = read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val &= ~(1 << 27);
	write32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Enable pll ldo */
	val = read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val |= (1 << 30);
	write32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);
	sdelay(5);

	/* Set default clk to 1008mhz */
	val = read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val &= ~((0x3 << 16) | (0xff << 8) | (0x3 << 0));
#ifdef CONFIG_CPU_FREQ
	val |= ((div(CONFIG_CPU_FREQ, 24000000) - 1) << 8);
#else
	val |= (41 << 8);
#endif
	write32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Lock enable */
	val = read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val |= (1 << 29);
	write32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Enable pll */
	val = read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val |= (1 << 31);
	write32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Wait pll stable */
	while (!(read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG) & (0x1 << 28)))
		;
	sdelay(20);

	/* Enable pll gating */
	val = read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val |= (1 << 27);
	write32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Lock disable */
	val = read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val &= ~(1 << 29);
	write32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);
	sdelay(1);

	/* set and change cpu clk src to PLL_CPUX, PLL_CPUX:AXI0 = 1008M:504M */
	val = read32(T113_CCU_BASE + CCU_CPU_AXI_CFG_REG);
	val &= ~(0x07 << 24 | 0x3 << 16 | 0x3 << 8 | 0xf << 0);
	val |= (0x03 << 24 | 0x0 << 16 | 0x3 << 8 | 0x1 << 0);
	write32(T113_CCU_BASE + CCU_CPU_AXI_CFG_REG, val);
	sdelay(1);
}

static void set_pll_periph0(void)
{
	uint32_t val;

	/* Periph0 has been enabled */
	if (read32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG) & (1 << 31))
		return;

	/* Change psi src to osc24m */
	val = read32(T113_CCU_BASE + CCU_PSI_CLK_REG);
	val &= (~(0x3 << 24));
	write32(val, T113_CCU_BASE + CCU_PSI_CLK_REG);

	/* Set default val */
	write32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG, 0x63 << 8);

	/* Lock enable */
	val = read32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	val |= (1 << 29);
	write32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG, val);

	/* Enabe pll 600m(1x) 1200m(2x) */
	val = read32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	val |= (1 << 31);
	write32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG, val);

	/* Wait pll stable */
	while (!(read32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG) & (0x1 << 28)))
		;
	sdelay(20);

	/* Lock disable */
	val = read32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	val &= ~(1 << 29);
	write32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG, val);
}

static void set_ahb(void)
{
	write32(T113_CCU_BASE + CCU_PSI_CLK_REG, (2 << 0) | (0 << 8));
	write32(T113_CCU_BASE + CCU_PSI_CLK_REG, read32(T113_CCU_BASE + CCU_PSI_CLK_REG) | (0x03 << 24));
	sdelay(1);
}

static void set_apb(void)
{
	write32(T113_CCU_BASE + CCU_APB0_CLK_REG, (2 << 0) | (1 << 8));
	write32(T113_CCU_BASE + CCU_APB0_CLK_REG, (0x03 << 24) | read32(T113_CCU_BASE + CCU_APB0_CLK_REG));
	sdelay(1);
}

static void set_dma(void)
{
	/* Dma reset */
	write32(T113_CCU_BASE + CCU_DMA_BGR_REG, read32(T113_CCU_BASE + CCU_DMA_BGR_REG) | (1 << 16));
	sdelay(20);
	/* Enable gating clock for dma */
	write32(T113_CCU_BASE + CCU_DMA_BGR_REG, read32(T113_CCU_BASE + CCU_DMA_BGR_REG) | (1 << 0));
}

static void set_mbus(void)
{
	uint32_t val;

	/* Reset mbus domain */
	val = read32(T113_CCU_BASE + CCU_MBUS_CLK_REG);
	val |= (0x1 << 30);
	write32(T113_CCU_BASE + CCU_MBUS_CLK_REG, val);
	sdelay(1);

	/* Enable mbus master clock gating */
	write32(T113_CCU_BASE + CCU_MBUS_MAT_CLK_GATING_REG, 0x00000d87);
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
	set_module(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	set_module(T113_CCU_BASE + CCU_PLL_VIDEO0_CTRL_REG);
	set_module(T113_CCU_BASE + CCU_PLL_VIDEO1_CTRL_REG);
	set_module(T113_CCU_BASE + CCU_PLL_VE_CTRL);
	set_module(T113_CCU_BASE + CCU_PLL_AUDIO0_CTRL_REG);
	set_module(T113_CCU_BASE + CCU_PLL_AUDIO1_CTRL_REG);
}

uint32_t sunxi_clk_get_peri1x_rate()
{
	uint32_t reg32;
	uint8_t	 plln, pllm, p0;

	/* PLL PERIx */
	reg32 = read32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0	 = ((reg32 >> 16) & 0x03) + 1;

		return ((div((24 * plln), (pllm * p0)) >> 1) * 1000 * 1000);
	}

	return 0;
}

#ifdef CONFIG_ENABLE_CPU_FREQ_DUMP
void sunxi_clk_dump()
{
	uint32_t reg32;
	//    uint32_t cpu_clk, plln, pllm;
	uint8_t p0, p1;

	/* PLL CPU */
	reg32 = read32(T113_CCU_BASE + CCU_CPU_AXI_CFG_REG);
#if 0
	cpu_clk = (reg32 >> 24) & 0x7;
	debug("freq: cpu_clock: ");
	switch(cpu_clk) {
		case 0x0:
		    debug("HOSC");
		    break;

		case 0x1:
		    debug("CLK32");
		    break;

		case 0x2:
		    debug("CLK16M_RC");
		    break;

		case 0x3:
		    debug("PLL_CPU");
		    break;

		case 0x4:
		    debug("PLL_PERI(1X)");
		    break;

		case 0x5:
		    debug("PLL_PERI(2X)");
		    break;

		case 0x6:
		    debug("PLL_PERI(800M)");
		    break;
	}
	debug("\r\n");
	debug("freq: PLL_cpu: ");
#endif
	reg32 = read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	//	if (reg32 & (1 << 31)) {
	p0 = (reg32 >> 16) & 0x03;
	if (p0 == 0) {
		p1 = 1;
	} else if (p0 == 1) {
		p1 = 2;
	} else if (p0 == 2) {
		p1 = 4;
	} else {
		p1 = 1;
	}

	debug("CPU freq: %u MHz\r\n", div((((reg32 >> 8) & 0xff) + 1) * 24, p1));

	//	} else {
	//	    debug("disabled\r\n");
	//	}

#if 0
	/* PLL PERIx */
	reg32 = read32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	if (reg32 & (1 << 31)) {
	    debug("freq: PLL_peri: enabled, ");

	    plln = ((reg32 >> 8) & 0xff) + 1;
	    pllm = (reg32 & 0x01) + 1;
	    p0 = ((reg32 >> 16) & 0x03) + 1;
	    p1 = ((reg32 >> 20) & 0x03) + 1;

	    debug("(2X)=%uMHz, ", div((24 * plln), (pllm * p0)));
	    debug("(1X)=%uMHz, ", div((24 * plln), (pllm * p0)) >> 1);
	    debug("(800M)=%uMHz\r\n", div((24 * plln), (pllm * p1)));

	} else {
	    debug("disabled\r\n");
	}

	/* PLL DDR */
	reg32 = read32(T113_CCU_BASE + CCU_PLL_DDR_CTRL_REG);
	if (reg32 & (1 << 31)) {
	    debug("freq: PLL_ddr: enabled");

	    plln = ((reg32 >> 8) & 0xff) + 1;

	    pllm = (reg32 & 0x01) + 1;
	    p1 = ((reg32 >> 1) & 0x1) + 1;
	    p0 = (reg32 & 0x01) + 1;

	    debug("=%uMHz\r\n", div((24 * plln), (p0 * p1)));

	} else {
	    debug("disabled\r\n");
	}
#endif
}
#endif
