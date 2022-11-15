#ifndef __SDHCI_H__
#define __SDHCI_H__

#include "sunxi_gpio.h"

typedef struct {
	u32_t cmdidx;
	u32_t cmdarg;
	u32_t resptype;
	u32_t response[4];
} sdhci_cmd_t;

typedef struct {
	u8_t * buf;
	u32_t flag;
	u32_t blksz;
	u32_t blkcnt;
} sdhci_data_t;


typedef struct {
	char *name;
	uint32_t addr;
	uint32_t pclk;
	uint32_t reset;
	u32_t voltage;
	u32_t width;
	u32_t clock;
	bool_t removable;
	bool_t isspi;

	gpio_mux_t gpio_d0;
	gpio_mux_t gpio_d1;
	gpio_mux_t gpio_d2;
	gpio_mux_t gpio_d3;
	gpio_mux_t gpio_cmd;
	gpio_mux_t gpio_clk;

	void * sdcard;
} sdhci_t;

extern sdhci_t sdhci0;


extern bool_t sdhci_reset(sdhci_t * hci);
extern bool_t sdhci_set_voltage(sdhci_t * hci, u32_t voltage);
extern bool_t sdhci_set_width(sdhci_t * hci, u32_t width);
extern bool_t sdhci_set_clock(sdhci_t * hci, u32_t clock);
extern bool_t sdhci_transfer(sdhci_t * hci, sdhci_cmd_t * cmd, sdhci_data_t * dat);
extern int sunxi_sdhci_init(sdhci_t *sdhci);

#endif /* __SDHCI_H__ */
