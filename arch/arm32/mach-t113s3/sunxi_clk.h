#ifndef __SUNXI_CLK_H__
#define __SUNXI_CLK_H__

#include "common.h"
#include "reg-ccu.h"
#include "reg-r-ccu.h"

void	 sunxi_clk_init(void);
uint32_t sunxi_clk_get_peri1x_rate(void);
uint32_t sunxi_clk_get_fail_addr(void);

void sunxi_clk_dump(void);

#endif
