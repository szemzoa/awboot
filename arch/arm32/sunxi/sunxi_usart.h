#ifndef __SUNXI_USART_H__
#define __SUNXI_USART_H__

#include "main.h"
#include "sunxi_gpio.h"
#include "ringbuffer.h"

#define USART_RX_BUFFER_SIZE 256

typedef struct {
	uint32_t	 base;
	uint8_t		 id;
	gpio_mux_t	 gpio_tx;
	gpio_mux_t	 gpio_rx;
	uint16_t	 irqn;
	char		 rx_buffer[USART_RX_BUFFER_SIZE];
	ringbuffer_t ringbuf;
} sunxi_usart_t;

void	 sunxi_usart_init(sunxi_usart_t *usart);
void	 sunxi_usart_putc(void *arg, char c);
uint16_t sunxi_usart_data_in_receive_buffer(sunxi_usart_t *usart);
uint8_t	 sunxi_usart_getbyte(sunxi_usart_t *usart);

#endif
