#include "debug.h"
#include "main.h"

extern void sys_uart_putc(void *arg, char c);

void debug(const char *fmt, ...) 
{
	va_list args;
	va_start(args, fmt);
	xvformat(sys_uart_putc, NULL, fmt, args);
	va_end(args);
}
