#ifndef __SUNXI_SPI_H__
#define __SUNXI_SPI_H__

#include "main.h"
#include "sunxi_gpio.h"

struct spinand_info_t {
	char *name;
	struct {
		uint8_t val[4];
		uint8_t len;
	} id;
	uint32_t page_size;
	uint32_t spare_size;
	uint32_t pages_per_block;
	uint32_t blocks_per_die;
	uint32_t planes_per_die;
	uint32_t ndies;
};

struct sunxi_spi_t {
	uint32_t    base;
	uint8_t     id;
	uint32_t    clk_rate;
	gpio_mux_t  gpio_cs;
	gpio_mux_t  gpio_sck;
	gpio_mux_t  gpio_miso;
	gpio_mux_t  gpio_mosi;

	struct spinand_info_t info;
};

extern int sunxi_spi_init(struct sunxi_spi_t *spi);
extern int spinand_detect(struct sunxi_spi_t *spi);
extern uint64_t spinand_read(struct sunxi_spi_t *spi, uint8_t *buf, uint64_t offset, uint64_t count);

#endif