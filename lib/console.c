#include "console.h"
#include "board.h"
#include "debug.h"
#include "strtok.h"
#include "sunxi_usart.h"
#include "main.h"

extern uint32_t get_sys_ticks(void);

console_t console;

static void cmd_echo(unsigned char argc, char **argv);

const cmd_t cmd_tbl[] = {
	{"echo", cmd_echo},
	{	 NULL,	   NULL}
};

uint32_t get_sys_ticks(void);

static void cmd_menu()
{
	message("awboot> ");
}

static int8_t cmd_parse(char *cmd)
{
	unsigned char argc, i = 0;
	char		 *argv[CONSOLE_ARGS_MAX];
	char		 *last;

	argv[i] = strtok_r(cmd, " ", &last);

	do {
		argv[++i] = strtok_r(NULL, " ", &last);

	} while ((i < CONSOLE_ARGS_MAX) && (argv[i] != NULL));

	argc = i;

	for (i = 0; cmd_tbl[i].cmd != NULL; i++) {
		if (!strcmp(argv[0], cmd_tbl[i].cmd)) {
			cmd_tbl[i].func(argc, argv);
			cmd_menu();
			return 0;
		}
	}

	message("unknown command [%s]\r\n", cmd);
	cmd_menu();

	return -1;
}

void console_init(sunxi_usart_t *usart)
{
	console.cmd_ptr = console.cmd;
	console.usart	= usart;

	message("\r\n");
	cmd_menu();
}

void console_handler(uint32_t timeout)
{
	char	 ch;
	uint32_t tmo = get_sys_ticks();

	while (1) {
		while (sunxi_usart_data_in_receive_buffer(console.usart) != 0) {
			ch		= sunxi_usart_getbyte(console.usart);
			timeout = CONSOLE_NO_TIMEOUT;
			{
				switch (ch) {
					case ASCII_CR:
						break;
					case ASCII_LF:
						message("\r\n");
						*console.cmd_ptr = '\0';
						if ((console.cmd_ptr - console.cmd) >= 1) {
							cmd_parse(console.cmd);
						} else {
							cmd_menu();
						}
						console.cmd_ptr = console.cmd;
						break;

					case ASCII_CTRL_C:
						message("\r\n");
						console.cmd_ptr = console.cmd;
						message("Aborted\r\n");

						cmd_menu();
						break;

					case ASCII_BKSPACE:
					case ASCII_DEL:
						if (console.cmd_ptr > console.cmd) {
							message("\b \b");
							console.cmd_ptr--;
						}
						break;

					default:
						message("%c", ch);
						*console.cmd_ptr++ = ch;
						if ((console.cmd_ptr - console.cmd) >= CONSOLE_BUFFER_SIZE) {
							*console.cmd_ptr = '\0';

							cmd_parse(console.cmd);
							console.cmd_ptr = console.cmd;
						}
						break;
				}
			}
		}

		if ((get_sys_ticks() - tmo) > timeout && timeout != CONSOLE_NO_TIMEOUT)
			return;
	}
}

static void cmd_echo(unsigned char argc, char **argv)
{
	message("%s\r\n", argv[1]);
}
