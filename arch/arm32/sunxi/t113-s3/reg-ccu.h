#ifndef __T113_REG_CCU_H__
#define __T113_REG_CCU_H__

#include "types.h"

#define CCU_BASE (0x02001000)

#define CCU_PLL_CPU_CTRL_REG		 (0x000)
#define CCU_PLL_DDR_CTRL_REG		 (0x010)
#define CCU_PLL_PERI0_CTRL_REG		 (0x020)
#define CCU_PLL_PERI1_CTRL_REG		 (0x028)
#define CCU_PLL_VIDEO0_CTRL_REG		 (0x040)
#define CCU_PLL_VIDEO1_CTRL_REG		 (0x048)
#define CCU_PLL_VE_CTRL				 (0x058)
#define CCU_PLL_AUDIO0_CTRL_REG		 (0x078)
#define CCU_PLL_AUDIO1_CTRL_REG		 (0x080)

#define CCU_CPU_AXI_CFG_REG			 (0x500)
#define CCU_CPU_GATING_REG			 (0x504)
#define CCU_PSI_CLK_REG				 (0x510)
#define CCU_AHB3_CLK_REG			 (0x51c)
#define CCU_APB0_CLK_REG			 (0x520)
#define CCU_APB1_CLK_REG			 (0x524)
#define CCU_MBUS_CLK_REG			 (0x540)
//#define CCU_DMA_BGR_REG				 (0x70c)
#define CCU_DRAM_CLK_REG			 (0x800)
#define CCU_MBUS_MAT_CLK_GATING_REG	 (0x804)
#define CCU_DRAM_BGR_REG			 (0x80c)


#endif /* __T113_REG_CCU_H__ */
