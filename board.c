#include "main.h"
#include "board.h"
#include "sunxi_gpio.h"
#include "sunxi_sdhci.h"
#include "sunxi_usart.h"
#include "sdcard.h"

dram_para_t ddr_param = {
	.dram_clk	 = 792,
	.dram_type	 = 3,
	.dram_zq	 = 0x7b7bfb,
	.dram_odt_en = 0x00,
	.dram_para1	 = 0x000010d2,
	.dram_para2	 = 0x0000,
	.dram_mr0	 = 0x1c70,
	.dram_mr1	 = 0x042,
	.dram_mr2	 = 0x18,
	.dram_mr3	 = 0x0,
	.dram_tpr0	 = 0x004A2195,
	.dram_tpr1	 = 0x02423190,
	.dram_tpr2	 = 0x0008B061,
	.dram_tpr3	 = 0xB4787896,
	.dram_tpr4	 = 0x0,
	.dram_tpr5	 = 0x48484848,
	.dram_tpr6	 = 0x00000048,
	.dram_tpr7	 = 0x1620121e,
	.dram_tpr8	 = 0x0,
	.dram_tpr9	 = 0x0,
	.dram_tpr10	 = 0x0,
	.dram_tpr11	 = 0x00340000,
	.dram_tpr12	 = 0x00000046,
	.dram_tpr13	 = 0x34000100,
};

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
	.gpio_clk = { GPIO_PIN(PORTF, 2), GPIO_PERIPH_MUX2 },
	.gpio_cmd = { GPIO_PIN(PORTF, 3), GPIO_PERIPH_MUX2 },
	.gpio_d0 = { GPIO_PIN(PORTF, 1), GPIO_PERIPH_MUX2 },
	.gpio_d1 = { GPIO_PIN(PORTF, 0), GPIO_PERIPH_MUX2 },
	.gpio_d2 = { GPIO_PIN(PORTF, 5), GPIO_PERIPH_MUX2 },
	.gpio_d3 = { GPIO_PIN(PORTF, 4), GPIO_PERIPH_MUX2 },
};

gpio_t	led_blue = GPIO_PIN(PORTD, 22);


void board_init_led(gpio_t led)
{
        sunxi_gpio_init(led, GPIO_OUTPUT);
        sunxi_gpio_set_value(led, 0);
}

int board_sdhci_init()
{
	sunxi_sdhci_init(&sdhci0);
	return sdcard_init(&sdcard0, &sdhci0);
}

void board_init()
{
	board_init_led(led_blue);
	sunxi_usart_init(&usart_dbg);
}
