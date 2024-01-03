#include "main.h"
#include "board-mangopi-dual.h"
#include "sunxi_gpio.h"
#include "sunxi_sdhci.h"
#include "sunxi_usart.h"
#include "sunxi_spi.h"
#include "sdmmc.h"

sunxi_usart_t usart5_dbg = {
	.base	 = 0x02501400,
	.id		 = 5,
	.gpio_tx = {GPIO_PIN(PORTB, 4), GPIO_PERIPH_MUX7},
	.gpio_rx = {GPIO_PIN(PORTB, 5), GPIO_PERIPH_MUX7},
};

sunxi_usart_t usart0_dbg = {
	.base	 = 0x02500000,
	.id		 = 0,
	.gpio_tx = {GPIO_PIN(PORTE, 2), GPIO_PERIPH_MUX6},
	.gpio_rx = {GPIO_PIN(PORTE, 3), GPIO_PERIPH_MUX6},
};

sunxi_usart_t usart3_dbg = {
	.base	 = 0x02500c00,
	.id		 = 3,
	.gpio_tx = {GPIO_PIN(PORTB, 6), GPIO_PERIPH_MUX7},
	.gpio_rx = {GPIO_PIN(PORTB, 7), GPIO_PERIPH_MUX7},
};

sunxi_spi_t sunxi_spi0 = {
	.base	   = 0x04025000,
	.id		   = 0,
	.mod_clk   = 200 * 1000 * 1000,
	.clk_rate  = 100 * 1000 * 1000,
	.gpio_cs   = {GPIO_PIN(PORTC, 3), GPIO_PERIPH_MUX2},
	.gpio_sck  = {GPIO_PIN(PORTC, 2), GPIO_PERIPH_MUX2},
	.gpio_mosi = {GPIO_PIN(PORTC, 4), GPIO_PERIPH_MUX2},
	.gpio_miso = {GPIO_PIN(PORTC, 5), GPIO_PERIPH_MUX2},
	.gpio_wp   = {GPIO_PIN(PORTC, 6), GPIO_PERIPH_MUX2},
	.gpio_hold = {GPIO_PIN(PORTC, 7), GPIO_PERIPH_MUX2},
};

sdhci_t sdhci0 = {
	.name	   = "sdhci0",
	.reg	   = (sdhci_reg_t *)0x04020000,
	.voltage   = MMC_VDD_27_36,
	.width	   = MMC_BUS_WIDTH_4,
	.clock	   = MMC_CLK_50M,
	.removable = 0,
	.isspi	   = FALSE,
	.gpio_clk  = {GPIO_PIN(PORTF, 2), GPIO_PERIPH_MUX2},
	.gpio_cmd  = {GPIO_PIN(PORTF, 3), GPIO_PERIPH_MUX2},
	.gpio_d0   = {GPIO_PIN(PORTF, 1), GPIO_PERIPH_MUX2},
	.gpio_d1   = {GPIO_PIN(PORTF, 0), GPIO_PERIPH_MUX2},
	.gpio_d2   = {GPIO_PIN(PORTF, 5), GPIO_PERIPH_MUX2},
	.gpio_d3   = {GPIO_PIN(PORTF, 4), GPIO_PERIPH_MUX2},
};

static gpio_t led_blue = GPIO_PIN(PORTD, 22);

void board_init_led(gpio_t led)
{
	sunxi_gpio_init(led, GPIO_OUTPUT);
	sunxi_gpio_set_value(led, 0);
}

int board_sdhci_init()
{
	sunxi_sdhci_init(&sdhci0);
	return sdmmc_init(&card0, &sdhci0);
}

void board_init()
{
	board_init_led(led_blue);
	sunxi_usart_init(&USART_DBG);
}
