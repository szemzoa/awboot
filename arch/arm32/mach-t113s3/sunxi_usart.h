#ifndef __SUNXI_USART_H__
#define __SUNXI_USART_H__

#include "common.h"
#include "sunxi_gpio.h"

// The UART registers base address.
#define UART_BASE(id) ((uint32_t)(0x02500000 + (0x400 * id)))

typedef struct {
	uint8_t	   id;
	gpio_mux_t gpio_tx;
	gpio_mux_t gpio_rx;
} sunxi_usart_t;

void sunxi_usart_init(const sunxi_usart_t *usart, uint32_t baudrate);
void sunxi_usart_putc(void *arg, char c);
void sunxi_usart_put(sunxi_usart_t *usart, char *in, uint32_t len);
void sunxi_usart_get(sunxi_usart_t *usart, char *out, uint32_t len);

#endif
