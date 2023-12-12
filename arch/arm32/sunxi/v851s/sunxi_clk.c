#include "main.h"
#include "board.h"
#include "sunxi_clk.h"
#include "reg-ccu.h"
#include "sunxi_ccu.h"
#include "debug.h"


void sunxi_clk_init(void)
{
	uint32_t val;

	/* cpu_clk = CPU_PLL/P, AXI_DIV = 2 */
	write32(CCU_BASE + CCU_CPU_CLK_REG, (0x3 << 8) | 0x01);
        sdelay(1);

	/* cpu_clk divider = 1 */
	val = read32(CCU_BASE + CCU_CPU_CLK_REG);
        val &= ~((1 << 16) | (1 << 17));
//        val |= (1 << 16);
        write32(CCU_BASE + CCU_CPU_CLK_REG, val);
        sdelay(5);

	/* CPU_PLL: enable LOCK, enable LDO, N = 38 * 24 = 912MHz */
        val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
        val &= ~((1 << 27) | (0x3FF << 8) | 0x3);			/* CPU_PLL: Output disable, PLL_N = 0, M = 0 */
        val |= (1 << 30 | (1 << 29));
#ifdef CONFIG_CPU_FREQ
	val |= (((CONFIG_CPU_FREQ / 24000000) - 1) << 8);
#else
	val |= (37 << 8);
#endif

        write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);
        sdelay(5);

	/* wait for PLL lock */
	while (!(read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG) & (0x1 << 28)))
		;

	sdelay(20);

	/* PLL lock disable, output enable */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
        val &= ~(1 << 29);
        val |= (1 << 27);
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* cpu clock = CPU_PLL / P, APB DIV = 4, AXI_DIV = 2 */
	val = read32(CCU_BASE + CCU_CPU_CLK_REG);
        val &= ~((0x7 << 24) | (0x3 << 8) | 0x3);
        val |= ((0x3 << 24) | (0x3 << 8) | 0x1);
	write32(CCU_BASE + CCU_CPU_CLK_REG, val);
	sdelay(1);

	/* Periph0 has been enabled */
	if (read32(CCU_BASE + CCU_PLL_PERI_CTRL_REG) & (1 << 31))
		debug("periph0 has been enabled\r\n");

	/* AHB_Clock = CLK_SRC/M/N, PERIPH_600M / N(1) / M(3) = 200MHz */
	write32(CCU_BASE + CCU_AHB_CLK_REG, (0x3 << 24) | 0x2);
	sdelay(1);

	/* APB0_Clock = CLK_SRC/M/N, PERIPH_600M / N(2) / M(3) = 100MHz */
	write32(CCU_BASE + CCU_APB0_CLK_REG, (0x3 << 24) | (1 << 8) | 0x2);
	sdelay(1);

	/* DMA reset */
	val = read32(CCU_BASE + CCU_DMA_BGR_REG);
	val |= (1 << 16);
	write32(CCU_BASE + CCU_DMA_BGR_REG, val);
	sdelay(20);

	/* enable DMA gate */
	val = read32(CCU_BASE + CCU_DMA_BGR_REG);
	val |= 0x01;
	write32(CCU_BASE + CCU_DMA_BGR_REG, val);
	sdelay(1);

	/* MBUS reset */
	val = read32(CCU_BASE + CCU_MBUS_CLK_REG);
	val |= (1 << 30);
	write32(CCU_BASE + CCU_MBUS_CLK_REG, val);
	sdelay(1);

	debug("sunxi clock init end\r\n");
	debug("cpu clk reg (#0x%x): 0x%lx\r\n", CCU_CPU_CLK_REG, read32(CCU_BASE + CCU_CPU_CLK_REG));

	return;
}

uint32_t sunxi_clk_get_peri1x_rate()
{
	uint32_t reg32;
	uint8_t	 plln, pllm, p0;

	/* PLL PERI */
	reg32 = read32(CCU_BASE + CCU_PLL_PERI_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0	 = ((reg32 >> 16) & 0x03) + 1;

		return ((((24 * plln) / (pllm * p0))) * 1000 * 1000);
	}

	return 0;
}

int spi_clk_init(sunxi_spi_t *spi)
{
        uint32_t rval;

        /* we use PERIPH_200M clock source */
        if (spi->mod_clk == 20000000000 ) {
            rval         = (1U << 31) | (0x2 << 24) | (0 << 8) | 0;     /* gate enable | use PERIPH_200M */
        } else {
            /* we use PERIPH_300M clock source */
            rval         = (1U << 31) | (0x1 << 24) | (0 << 8) | 0;     /* gate enable | use PERIPH_300M */
        }
        trace("SPI: parent_clk=%ldMHz\r\n", spi->mod_clk);

        write32(CCU_BASE + CCU_SPI0_CLK_REG, rval);

        return 0;
}

#ifdef CONFIG_ENABLE_CPU_FREQ_DUMP
void sunxi_clk_dump()
{
	uint32_t	reg32;
	uint32_t	cpu_clk_src;
	uint32_t UNUSED_DEBUG plln, pllm;
	uint8_t		p0;
	uint8_t  UNUSED_DEBUG p1;
	const char UNUSED_DEBUG *clock_str;


	/* PLL CPU */
	reg32		= read32(CCU_BASE + CCU_CPU_CLK_REG);
	cpu_clk_src = (reg32 >> 24) & 0x7;
	debug("CLK: CPU CLK_reg=0x%lx\r\n", reg32);

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
			clock_str = "PLL_PERI_600M";
			break;

		case 0x5:
			clock_str = "PLL_PERI_800M";
			break;

		default:
			clock_str = "ERROR";
	}

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

	debug("CLK: CPU PLL=%s FREQ=%luMHz\r\n", clock_str, ((((read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG) >> 8) & 0xff) + 1) * 24 / p1));

	/* PLL PERI */
	reg32 = read32(CCU_BASE + CCU_PLL_PERI_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0	 = ((reg32 >> 16) & 0x03) + 1;
		p1	 = ((reg32 >> 20) & 0x03) + 1;

		debug("CLK: PLL_peri (2X)=%luMHz, (1X)=%luMHz, (800M)=%luMHz\r\n", (24 * plln) / (pllm * p0),
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

		debug("CLK: PLL_ddr=%luMHz\r\n", (24 * plln) / (p0 * p1));

	} else {
		debug("CLK: PLL_ddr disabled\r\n");
	}
}
#endif
