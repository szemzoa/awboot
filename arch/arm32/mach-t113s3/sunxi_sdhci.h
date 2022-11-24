#ifndef __SDHCI_H__
#define __SDHCI_H__

#include "sunxi_gpio.h"

typedef struct {
	uint32_t idx;
	uint32_t arg;
	uint32_t resptype;
	uint32_t response[4];
} sdhci_cmd_t;

typedef struct {
	uint8_t *buf;
	uint32_t flag;
	uint32_t blksz;
	uint32_t blkcnt;
} sdhci_data_t;

typedef struct {
	char	 *name;
	uint32_t addr;
	uint32_t pclk;
	uint32_t reset;
	uint32_t voltage;
	uint32_t width;
	uint32_t clock;
	bool_t	 removable;
	bool_t	 isspi;

	gpio_mux_t gpio_d0;
	gpio_mux_t gpio_d1;
	gpio_mux_t gpio_d2;
	gpio_mux_t gpio_d3;
	gpio_mux_t gpio_cmd;
	gpio_mux_t gpio_clk;

} sdhci_t;

extern sdhci_t sdhci0;

bool_t sdhci_reset(sdhci_t *hci);
bool_t sdhci_set_voltage(sdhci_t *hci, uint32_t voltage);
bool_t sdhci_set_width(sdhci_t *hci, uint32_t width);
bool_t sdhci_set_clock(sdhci_t *hci, uint32_t clock);
bool_t sdhci_transfer(sdhci_t *hci, sdhci_cmd_t *cmd, sdhci_data_t *dat);
int	   sunxi_sdhci_init(sdhci_t *sdhci);

#endif /* __SDHCI_H__ */
