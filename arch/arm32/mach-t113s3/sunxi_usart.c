#include <stdarg.h>
#include "io.h"
#include "common.h"
#include "sunxi_usart.h"
#include "reg-ccu.h"

// Macros to access UART registers.
#define UART_RBR(n) *(volatile uint32_t *)(UART_BASE(n) + 0x00)
#define UART_THR(n) *(volatile uint32_t *)(UART_BASE(n) + 0x00)
#define UART_DLL(n) *(volatile uint32_t *)(UART_BASE(n) + 0x00)
#define UART_IER(n) *(volatile uint32_t *)(UART_BASE(n) + 0x04)
#define UART_FCR(n) *(volatile uint32_t *)(UART_BASE(n) + 0x08)
#define UART_LCR(n) *(volatile uint32_t *)(UART_BASE(n) + 0x0C)
#define UART_MCR(n) *(volatile uint32_t *)(UART_BASE(n) + 0x10)
#define UART_LSR(n) *(volatile uint32_t *)(UART_BASE(n) + 0x14)
#define UART_USR(n) *(volatile uint32_t *)(UART_BASE(n) + 0x7C)

#define UART_USR_BUSY (0x1 << 0)
#define UART_USR_TFNF (0x1 << 1)
#define UART_USR_TFE  (0x1 << 2)
#define UART_USR_RFNE (0x1 << 3)
#define UART_USR_RFF  (0x1 << 4)

void sunxi_usart_init(const sunxi_usart_t *usart, uint32_t baudrate)
{
	uint32_t addr;
	uint32_t val;
	uint32_t div = 13;

	/* Config usart TXD and RXD pins */
	sunxi_gpio_init(usart->gpio_tx.pin, usart->gpio_tx.mux);
	sunxi_gpio_init(usart->gpio_rx.pin, usart->gpio_rx.mux);
	sunxi_gpio_set_pull(usart->gpio_tx.pin, GPIO_PULL_UP);
	sunxi_gpio_set_pull(usart->gpio_rx.pin, GPIO_PULL_UP);

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

	div = 1500000 / baudrate;
	if (div == 0)
		div = 1;

	// Configure baud rate
	UART_LCR(usart->id) = (1 << 7) | 3;
	UART_DLL(usart->id) = div;
	UART_LCR(usart->id) = 3;

	// Enable FIFO
	UART_FCR(usart->id) = 0x00000001;
}

void sunxi_usart_putc(void *arg, char c)
{
	sunxi_usart_t *usart = (sunxi_usart_t *)arg;

	while ((UART_USR(usart->id) & UART_USR_TFNF) == 0)
		;
	UART_THR(usart->id) = c;
	while ((UART_USR(usart->id) & UART_USR_BUSY) == 1)
		;
}

void sunxi_usart_put(sunxi_usart_t *usart, char *in, uint32_t len)
{
	while (len--) {
		sunxi_usart_putc(usart, *in);
		in++;
	}
}

void sunxi_usart_get(sunxi_usart_t *usart, char *out, uint32_t len)
{
	while (len--) {
		while ((UART_USR(usart->id) & UART_USR_RFNE) == 0) {
		};
		*out++ = UART_RBR(usart->id) & 0xff;
	}
}
