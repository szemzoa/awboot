#include "main.h"
#include "sunxi_gpio.h"
#include "sunxi_sdhci.h"
#include "sunxi_usart.h"
#include "sdcard.h"

struct sunxi_usart_t usart_dbg = {
	.base 	= 0x02500000,
	.id 	= 0,
	.gpio_tx = { GPIO_PIN(PORTE, 2), GPIO_PERIPH_MUX6 },
	.gpio_rx = { GPIO_PIN(PORTE, 3), GPIO_PERIPH_MUX6 },
};

struct sdhci_t sdhci0 = {
	.name 	= "sdhci0",
	.addr 	= 0x04020000,
	.reset 	= 400,
	.voltage = MMC_VDD_27_36,
	.width 	= MMC_BUS_WIDTH_4,
	.clock 	= 25 * 1000 * 1000,
	.removable = 0,
	.isspi 	= FALSE,
	.gpio_clk = GPIO_PIN(PORTF, 2),
	.gpio_cmd = GPIO_PIN(PORTF, 3),
	.gpio_d0 = GPIO_PIN(PORTF, 1),
	.gpio_d1 = GPIO_PIN(PORTF, 0),
	.gpio_d2 = GPIO_PIN(PORTF, 5),
	.gpio_d3 = GPIO_PIN(PORTF, 4),
};

gpio_t	led_blue = GPIO_PIN(PORTD, 22);


void board_init_led(gpio_t led)
{
        sunxi_gpio_init(led, GPIO_OUTPUT);
        sunxi_gpio_set_value(led, 0);
}

void board_init()
{
	board_init_led(led_blue);

	sunxi_usart_init(&usart_dbg);

	sunxi_sdhci_init(&sdhci0);
	sdcard_init(&sdcard0, &sdhci0);
}
