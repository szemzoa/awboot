#ifndef __SUNXI_GPIO_H__
#define __SUNXI_GPIO_H__

#include <inttypes.h>

enum {
	GPIO_INPUT		  = 0,
	GPIO_OUTPUT		  = 1,
	GPIO_PERIPH_MUX2  = 2,
	GPIO_PERIPH_MUX3  = 3,
	GPIO_PERIPH_MUX4  = 4,
	GPIO_PERIPH_MUX5  = 5,
	GPIO_PERIPH_MUX6  = 6,
	GPIO_PERIPH_MUX7  = 7,
	GPIO_PERIPH_MUX8  = 8,
	GPIO_PERIPH_MUX14 = 14,
	GPIO_DISABLED	  = 0xf,
};

#define PORTA			 0
#define PORTB			 1
#define PORTC			 2
#define PORTD			 3
#define PORTE			 4
#define PORTF			 5
#define PORTG			 6
#define PORTH			 7
#define PORTI			 8
#define SUNXI_GPIO_PORTS (PORTI + 1)

enum gpio_pull_t {
	GPIO_PULL_UP   = 0,
	GPIO_PULL_DOWN = 1,
	GPIO_PULL_NONE = 2,
};

typedef uint32_t gpio_t;
#define PIO_NUM_IO_BITS 5

#define GPIO_PIN(x, y) (((uint32_t)(x << PIO_NUM_IO_BITS)) | y)

typedef struct {
	gpio_t	pin;
	uint8_t mux;
} gpio_mux_t;

extern void sunxi_gpio_init(gpio_t pin, int cfg);
extern void sunxi_gpio_set_value(gpio_t pin, int value);
extern int	sunxi_gpio_read(gpio_t pin);
extern void sunxi_gpio_set_pull(gpio_t pin, enum gpio_pull_t pull);

#endif