#ifndef __SUNXI_CLK_H__
#define __SUNXI_CLK_H__

#include "main.h"

extern void		sunxi_clk_init(void);
extern uint32_t sunxi_clk_get_peri1x_rate(void);

extern void sunxi_clk_dump(void);

#endif
