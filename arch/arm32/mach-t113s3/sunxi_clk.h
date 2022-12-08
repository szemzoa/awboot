#ifndef __SUNXI_CLK_H__
#define __SUNXI_CLK_H__

#include "main.h"
#include "reg-ccu.h"

void	 sunxi_clk_init(void);
uint32_t sunxi_clk_get_peri1x_rate(void);

void sunxi_clk_dump(void);

#endif
