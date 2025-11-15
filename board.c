#include "common.h"
#include "board.h"
#include "sunxi_gpio.h"
#include "sunxi_sdhci.h"
#include "sunxi_usart.h"
#include "sunxi_spi.h"
#include "sunxi_wdg.h"
#include "sdmmc.h"

sunxi_usart_t usart0_dbg = {
	.id		 = 0,
	.gpio_tx = {GPIO_PIN(PORTE, 2), GPIO_PERIPH_MUX6},
	.gpio_rx = {GPIO_PIN(PORTE, 3), GPIO_PERIPH_MUX6},
};

sunxi_usart_t usart5_dbg = {
	.id		 = 5,
	.gpio_tx = {GPIO_PIN(PORTB, 4), GPIO_PERIPH_MUX7},
	.gpio_rx = {GPIO_PIN(PORTB, 5), GPIO_PERIPH_MUX7},
};

sunxi_usart_t usart3_dbg = {
	.id		 = 3,
	.gpio_tx = {GPIO_PIN(PORTB, 6), GPIO_PERIPH_MUX7},
	.gpio_rx = {GPIO_PIN(PORTB, 7), GPIO_PERIPH_MUX7},
};

sunxi_spi_t sunxi_spi0 = {
	.base	   = 0x04025000,
	.id		   = 0,
	.clk_rate  = 100 * 1000 * 1000,
	.gpio_cs   = {GPIO_PIN(PORTC, 3), GPIO_PERIPH_MUX2},
	.gpio_sck  = {GPIO_PIN(PORTC, 2), GPIO_PERIPH_MUX2},
	.gpio_mosi = {GPIO_PIN(PORTC, 4), GPIO_PERIPH_MUX2},
	.gpio_miso = {GPIO_PIN(PORTC, 5), GPIO_PERIPH_MUX2},
	.gpio_wp   = {GPIO_PIN(PORTC, 6), GPIO_PERIPH_MUX2},
	.gpio_hold = {GPIO_PIN(PORTC, 7), GPIO_PERIPH_MUX2},
};

sdhci_t sdhci0 = {
	.name	        = "sdhci0",
	.reg	        = (sdhci_reg_t *)0x04020000,
  .id           = 0,
	.voltage      = MMC_VDD_27_36,
	.width        = MMC_BUS_WIDTH_4,
	.clock_wanted = MMC_CLK_50M,
	.removable    = 0,
	.isspi        = FALSE,
	.gpio_clk     = {GPIO_PIN(PORTF, 2), GPIO_PERIPH_MUX2},
	.gpio_cmd     = {GPIO_PIN(PORTF, 3), GPIO_PERIPH_MUX2},
	.gpio_d0      = {GPIO_PIN(PORTF, 1), GPIO_PERIPH_MUX2},
	.gpio_d1      = {GPIO_PIN(PORTF, 0), GPIO_PERIPH_MUX2},
	.gpio_d2      = {GPIO_PIN(PORTF, 5), GPIO_PERIPH_MUX2},
	.gpio_d3      = {GPIO_PIN(PORTF, 4), GPIO_PERIPH_MUX2},
};

// eMMC on SMHC2
sdhci_t sdhci2 = {
	.name		      = "sdhci2",
	.reg		      = (sdhci_reg_t *)0x04022000,
  .id           = 2,
	.voltage	    = MMC_VDD_27_36,
	.width		    = MMC_BUS_WIDTH_4,
	.clock_wanted = MMC_CLK_50M,
	.removable	  = 0,
	.isspi		    = FALSE,
	.gpio_clk	    = {GPIO_PIN(PORTC, 2), GPIO_PERIPH_MUX3},
	.gpio_cmd	    = {GPIO_PIN(PORTC, 3), GPIO_PERIPH_MUX3},
	.gpio_d0	    = {GPIO_PIN(PORTC, 6), GPIO_PERIPH_MUX3},
	.gpio_d1	    = {GPIO_PIN(PORTC, 5), GPIO_PERIPH_MUX3},
	.gpio_d2	    = {GPIO_PIN(PORTC, 4), GPIO_PERIPH_MUX3},
	.gpio_d3	    = {GPIO_PIN(PORTC, 7), GPIO_PERIPH_MUX3},
};

static const gpio_t mmc_rst = GPIO_PIN(PORTF, 6);
static gpio_t led_blue = GPIO_PIN(PORTD, 22);

static void board_reset_mmc(void)
{
	/* Assert reset, wait, then deassert to guarantee a clean start */
	sunxi_gpio_write(mmc_rst, 1);
	mdelay(1);
	sunxi_gpio_write(mmc_rst, 0);
	mdelay(10);
}

static void output_init(const gpio_t gpio)
{
	sunxi_gpio_init(gpio, GPIO_OUTPUT);
	sunxi_gpio_write(gpio, 0);
};

static void intput_init(const gpio_t gpio)
{
	sunxi_gpio_init(gpio, GPIO_INPUT);
	sunxi_gpio_set_pull(gpio, GPIO_PULL_UP);
};

void board_init_led(gpio_t led)
{
	sunxi_gpio_init(led, GPIO_OUTPUT);
	sunxi_gpio_write(led, 0);
}

void board_set_led(uint8_t num, uint8_t on)
{
	switch (num) {
		case LED_BOARD:
			sunxi_gpio_write(led_blue, on);
			break;
		default:
			break;
	}
}

void board_init()
{
	board_init_led(led_blue);
	sunxi_usart_init(&USART_DBG, 115200);
}
