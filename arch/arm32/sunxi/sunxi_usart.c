#include <stdarg.h>
#include "io.h"
#include "irq.h"
#include "main.h"
#include "board.h"
#include "sunxi_usart.h"
#include "sunxi_ccu.h"
#include "reg-ccu.h"

#ifdef CONFIG_ENABLE_CONSOLE
void sunxi_uart_irq_handler(void *arg);
#endif

void sunxi_usart_init(sunxi_usart_t *usart)
{
	uint32_t addr;
	uint32_t val;

	/* Config usart TXD and RXD pins */
	sunxi_gpio_init(usart->gpio_tx.pin, usart->gpio_tx.mux);
	sunxi_gpio_init(usart->gpio_rx.pin, usart->gpio_rx.mux);

	/* Open the clock gate for usart */
	addr = CCU_BASE + CCU_UART_BGR_REG;
	val	 = read32(addr);
	val |= 1 << usart->id;
	write32(addr, val);

	/* Deassert USART reset */
	addr = CCU_BASE + CCU_UART_BGR_REG;
	val	 = read32(addr);
	val |= 1 << (16 + usart->id);
	write32(addr, val);

	/* Config USART to 115200-8-1-0 */
	addr = usart->base;
	write32(addr + 0x04, 0x0);
	write32(addr + 0x08, 0xf7);
	write32(addr + 0x10, 0x0);
	val = read32(addr + 0x0c);
	val |= (1 << 7);
	write32(addr + 0x0c, val);
	write32(addr + 0x00, 0xd & 0xff);
	write32(addr + 0x04, (0xd >> 8) & 0xff);
	val = read32(addr + 0x0c);
	val &= ~(1 << 7);
	write32(addr + 0x0c, val);
	val = read32(addr + 0x0c);
	val &= ~0x1f;
	val |= (0x3 << 0) | (0 << 2) | (0x0 << 3);
	write32(addr + 0x0c, val);

#ifdef CONFIG_ENABLE_CONSOLE
	ringbuffer_init(&usart->ringbuf, usart->rx_buffer, sizeof(usart->rx_buffer));

	gic400_set_irq_handler(usart->irqn, sunxi_uart_irq_handler, (void *)usart);
	gic400_irq_enable(usart->irqn);

	// enable RX irq
	write32(addr + 0x04, 0x01);
#endif
}

void sunxi_usart_putc(void *arg, char c)
{
	sunxi_usart_t *usart = (sunxi_usart_t *)arg;

	while (!(read32(usart->base + 0x14) & (0x1 << 5)))
		;

	write32(usart->base + 0x00, c);

	while ((read32(usart->base + 0x14) & (0x1 << 6)))
		;
}

#ifdef CONFIG_ENABLE_CONSOLE
void sunxi_uart_irq_handler(void *arg)
{
	sunxi_usart_t *usart = (sunxi_usart_t *)arg;

	uint32_t val = read32(usart->base);
	ringbuffer_put(&usart->ringbuf, (uint8_t)val);
}

uint16_t sunxi_usart_data_in_receive_buffer(sunxi_usart_t *usart)
{
	return ringbuffer_num(&usart->ringbuf);
}

uint8_t sunxi_usart_getbyte(sunxi_usart_t *usart)
{
	uint8_t ans;

	ringbuffer_get(&usart->ringbuf, &ans);

	return ans;
}
#endif
