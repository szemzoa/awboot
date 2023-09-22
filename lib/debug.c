#include "debug.h"
#include "sunxi_usart.h"
#include "main.h"
#include "board.h"

void message(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	xvformat(sunxi_usart_putc, &USART_DBG, fmt, args);
	va_end(args);
}
