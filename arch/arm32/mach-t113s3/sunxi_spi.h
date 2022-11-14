#ifndef __SUNXI_SPI_H__
#define __SUNXI_SPI_H__

#include "main.h"
#include "sunxi_gpio.h"

typedef struct {
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
} spi_nand_info_t;

typedef struct {
	uint32_t    base;
	uint8_t     id;
	uint32_t    clk_rate;
	gpio_mux_t  gpio_cs;
	gpio_mux_t  gpio_sck;
	gpio_mux_t  gpio_miso;
	gpio_mux_t  gpio_mosi;
	gpio_mux_t  gpio_wp;
	gpio_mux_t  gpio_hold;

	spi_nand_info_t info;
} sunxi_spi_t;

typedef enum {
	SPI_IO_MODE_SINGLE = 0x00,
	SPI_IO_MODE_DUAL_RX,
	SPI_IO_MODE_DUAL_IO,
	SPI_IO_MODE_QUAD_RX,
	SPI_IO_MODE_QUAD_IO,
} spi_io_mode_t;

int sunxi_spi_init(sunxi_spi_t *spi);
void sunxi_spi_disable(sunxi_spi_t *spi);
int spi_nand_detect(sunxi_spi_t *spi);
uint64_t spi_nand_read(sunxi_spi_t *spi, spi_io_mode_t mode, uint8_t *buf, uint64_t offset, uint64_t count);

#endif