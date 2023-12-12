#ifndef __CCU_H__
#define __CCU_H__

#define CCU_BASE (0x02001000)

#define CCU_PLL_CPU_CTRL_REG	0x0000	/* PLL_CPU Control Register */
#define CCU_PLL_DDR_CTRL_REG	0x0010	/* PLL_DDR Control Register */
#define CCU_PLL_PERI_CTRL_REG	0x0020	/* PLL_PERI Control Register */

#define CCU_CPU_CLK_REG		0x0500	/* CPU Clock Register */
#define CCU_CPU_GATING_REG	0x0504	/* CPU Gating Configuration Register */
#define CCU_AHB_CLK_REG		0x0510	/* AHB Clock Register */
#define CCU_APB0_CLK_REG	0x0520	/* APB0 Clock Register */
#define CCU_APB1_CLK_REG	0x0524	/* APB1 Clock Register */
#define CCU_MBUS_CLK_REG	0x0540	/* MBUS Clock Register */

#define CCU_DRAM_CLK_REG	0x0800	/* DRAM Clock Register */
#define CCU_DRAM_BGR_REG	0x080C	/* DRAM Bus Gating Reset Register */

#endif
