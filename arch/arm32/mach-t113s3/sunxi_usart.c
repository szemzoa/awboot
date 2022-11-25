#include <stdarg.h>
#include "io.h"
#include "main.h"
#include "sunxi_usart.h"
#include "reg-ccu.h"

void sunxi_usart_init(sunxi_usart_t *usart)
{
	uint32_t addr;
	uint32_t val;

	/* Config usart TXD and RXD pins */
	sunxi_gpio_init(usart->gpio_tx.pin, usart->gpio_tx.mux);
	sunxi_gpio_init(usart->gpio_rx.pin, usart->gpio_rx.mux);

	/* Open the clock gate for usart */
	addr = T113_CCU_BASE + CCU_USART_BGR_REG;
	val	 = read32(addr);
	val |= 1 << usart->id;
	write32(addr, val);

	/* Deassert USART reset */
	addr = T113_CCU_BASE + CCU_USART_BGR_REG;
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
}

void sunxi_usart_putc(void *arg, char c)
{
	sunxi_usart_t *usart = (sunxi_usart_t *)arg;

	while ((read32(usart->base + 0x7c) & (0x1 << 1)) == 0)
		;
	write32(usart->base + 0x00, c);
	while ((read32(usart->base + 0x7c) & (0x1 << 0)) == 1)
		;
}
