#ifndef __SUNXI_CLK_H__
#define __SUNXI_CLK_H__

#include "main.h"
#include "sunxi_spi.h"
#include "reg-ccu.h"

void	 sunxi_clk_init(void);
uint32_t sunxi_clk_get_peri1x_rate(void);
int spi_clk_init(sunxi_spi_t *spi);

void sunxi_clk_dump(void);

#endif
