#ifndef __SUNXI_USART_H__
#define __SUNXI_USART_H__

#include "main.h"
#include "sunxi_gpio.h"

typedef struct {
	uint32_t   base;
	uint8_t	   id;
	gpio_mux_t gpio_tx;
	gpio_mux_t gpio_rx;
} sunxi_usart_t;

extern void sunxi_usart_init(sunxi_usart_t *usart);
extern void sunxi_usart_putc(void *arg, char c);

#endif
