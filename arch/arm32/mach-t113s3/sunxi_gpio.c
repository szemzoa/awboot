#include "main.h"
#include "debug.h"
#include "sunxi_gpio.h"

enum {
	GPIO_CFG0 = 0x00,
	GPIO_CFG1 = 0x04,
	GPIO_CFG2 = 0x08,
	GPIO_CFG3 = 0x0c,
	GPIO_DAT  = 0x10,
	GPIO_DRV0 = 0x14,
	GPIO_DRV1 = 0x18,
	GPIO_DRV2 = 0x1c,
	GPIO_DRV3 = 0x20,
	GPIO_PUL0 = 0x24,
	GPIO_PUL1 = 0x28,
};

static inline uint32_t _port_num(gpio_t pin)
{
	return pin >> PIO_NUM_IO_BITS;
}

static uint32_t _port_base_get(gpio_t pin)
{
	uint32_t port = pin >> PIO_NUM_IO_BITS;

	switch (port) {
		case PORTB:
			return 0x02000030;
			break;
		case PORTC:
			return 0x02000060;
			break;
		case PORTD:
			return 0x02000090;
			break;
		case PORTE:
			return 0x020000c0;
			break;
		case PORTF:
			return 0x020000f0;
			break;
		case PORTG:
			return 0x02000120;
			break;
	}

	return 0;
}

static inline uint32_t _pin_num(gpio_t pin)
{
	return (pin & ((1 << PIO_NUM_IO_BITS) - 1));
}

void sunxi_gpio_init(gpio_t pin, int cfg)
{
	uint32_t port_addr = _port_base_get(pin);
	uint32_t pin_num   = _pin_num(pin);
	uint32_t addr;
	uint32_t val;

	addr = port_addr + GPIO_CFG0 + ((pin_num >> 3) << 2);
	val	 = read32(addr);
	val &= ~(0xf << ((pin_num & 0x7) << 2));
	val |= ((cfg & 0xf) << ((pin_num & 0x7) << 2));
	write32(addr, val);
}

void sunxi_gpio_set_value(gpio_t pin, int value)
{
	uint32_t port_addr = _port_base_get(pin);
	uint32_t pin_num   = _pin_num(pin);
	uint32_t val;

	val = read32(port_addr + GPIO_DAT);
	val &= ~(1 << pin_num);
	val |= (!!value) << pin_num;
	write32(port_addr + GPIO_DAT, val);
}

int sunxi_gpio_read(gpio_t pin)
{
	uint32_t port_addr = _port_base_get(pin);
	uint32_t pin_num   = _pin_num(pin);
	uint32_t val;

	val = read32(port_addr + GPIO_DAT);
	return !!(val & (1 << pin_num));
}

void sunxi_gpio_set_pull(gpio_t pin, enum gpio_pull_t pull)
{
	uint32_t port_addr = _port_base_get(pin);
	uint32_t pin_num   = _pin_num(pin);
	uint32_t addr;
	uint32_t val, v;

	switch (pull) {
		case GPIO_PULL_UP:
			v = 0x1;
			break;

		case GPIO_PULL_DOWN:
			v = 0x2;
			break;

		case GPIO_PULL_NONE:
			v = 0x0;
			break;

		default:
			v = 0x0;
			break;
	}

	addr = port_addr + GPIO_PUL0 + ((pin_num >> 4) << 2);
	val	 = read32(addr);
	val &= ~(v << ((pin_num & 0xf) << 1));
	val |= (v << ((pin_num & 0xf) << 1));
	write32(addr, val);
}

#if 0

static int gpio_t113_get_cfg(struct gpiochip_t * chip, int offset)
{
	struct gpio_t113_pdata_t * pdat = (struct gpio_t113_pdata_t *)chip->priv;
	virtual_addr_t addr;
	u32_t val;

	if(offset >= chip->ngpio)
		return 0;

	addr = pdat->virt + GPIO_CFG0 + ((offset >> 3) << 2);
	val = (read32(addr) >> ((offset & 0x7) << 2)) & 0xf;
	return val;
}

static void gpio_t113_set_pull(struct gpiochip_t * chip, int offset, enum gpio_pull_t pull)
{
	struct gpio_t113_pdata_t * pdat = (struct gpio_t113_pdata_t *)chip->priv;
	virtual_addr_t addr;
	u32_t val, v;

	if(offset >= chip->ngpio)
		return;

	switch(pull)
	{
	case GPIO_PULL_UP:
		v = 0x1;
		break;

	case GPIO_PULL_DOWN:
		v = 0x2;
		break;

	case GPIO_PULL_NONE:
		v = 0x0;
		break;

	default:
		v = 0x0;
		break;
	}

	addr = pdat->virt + GPIO_PUL0 + ((offset >> 4) << 2);
	val = read32(addr);
	val &= ~(v << ((offset & 0xf) << 1));
	val |= (v << ((offset & 0xf) << 1));
	write32(addr, val);
}

static enum gpio_pull_t gpio_t113_get_pull(struct gpiochip_t * chip, int offset)
{
	struct gpio_t113_pdata_t * pdat = (struct gpio_t113_pdata_t *)chip->priv;
	virtual_addr_t addr;
	u32_t v = 0;

	if(offset >= chip->ngpio)
		return GPIO_PULL_NONE;

	addr = pdat->virt + GPIO_PUL0 + ((offset >> 4) << 2);
	v = (read32(addr) >> ((offset & 0xf) << 1)) & 0x3;

	switch(v)
	{
	case 0:
		return GPIO_PULL_NONE;
	case 1:
		return GPIO_PULL_UP;
	case 2:
		return GPIO_PULL_DOWN;
	default:
		break;
	}
	return GPIO_PULL_NONE;
}

static void gpio_t113_set_drv(struct gpiochip_t * chip, int offset, enum gpio_drv_t drv)
{
	struct gpio_t113_pdata_t * pdat = (struct gpio_t113_pdata_t *)chip->priv;
	virtual_addr_t addr;
	u32_t val, v;

	if(offset >= chip->ngpio)
		return;

	switch(drv)
	{
	case GPIO_DRV_WEAK:
		v = 0x0;
		break;

	case GPIO_DRV_WEAKER:
		v = 0x1;
		break;

	case GPIO_DRV_STRONGER:
		v = 0x2;
		break;

	case GPIO_DRV_STRONG:
		v = 0x3;
		break;

	default:
		v = 0x0;
		break;
	}

	addr = pdat->virt + GPIO_DRV0 + ((offset >> 3) << 2);
	val = read32(addr);
	val &= ~(v << ((offset & 0x7) << 2));
	val |= (v << ((offset & 0x7) << 2));
	write32(addr, val);
}

static enum gpio_drv_t gpio_t113_get_drv(struct gpiochip_t * chip, int offset)
{
	struct gpio_t113_pdata_t * pdat = (struct gpio_t113_pdata_t *)chip->priv;
	virtual_addr_t addr;
	u32_t v = 0;

	if(offset >= chip->ngpio)
		return GPIO_DRV_WEAK;

	addr = pdat->virt + GPIO_DRV0 + ((offset >> 3) << 2);
	v = (read32(addr) >> ((offset & 0x7) << 2)) & 0x3;

	switch(v)
	{
	case 0:
		return GPIO_DRV_WEAK;
	case 1:
		return GPIO_DRV_WEAKER;
	case 2:
		return GPIO_DRV_STRONGER;
	case 3:
		return GPIO_DRV_STRONG;
	default:
		break;
	}
	return GPIO_DRV_WEAK;
}

static void gpio_t113_set_rate(struct gpiochip_t * chip, int offset, enum gpio_rate_t rate)
{
}

static enum gpio_rate_t gpio_t113_get_rate(struct gpiochip_t * chip, int offset)
{
	return GPIO_RATE_SLOW;
}

static void gpio_t113_set_dir(struct gpiochip_t * chip, int offset, enum gpio_direction_t dir)
{
	if(offset >= chip->ngpio)
		return;

	switch(dir)
	{
	case GPIO_DIRECTION_INPUT:
		gpio_t113_set_cfg(chip, offset, 0);
		break;

	case GPIO_DIRECTION_OUTPUT:
		gpio_t113_set_cfg(chip, offset, 1);
		break;

	default:
		break;
	}
}

static enum gpio_direction_t gpio_t113_get_dir(struct gpiochip_t * chip, int offset)
{
	if(offset >= chip->ngpio)
		return GPIO_DIRECTION_INPUT;

	switch(gpio_t113_get_cfg(chip, offset))
	{
	case 0:
		return GPIO_DIRECTION_INPUT;
	case 1:
		return GPIO_DIRECTION_OUTPUT;
	default:
		break;
	}
	return GPIO_DIRECTION_INPUT;
}

static void gpio_t113_set_value(struct gpiochip_t * chip, int offset, int value)
{
	struct gpio_t113_pdata_t * pdat = (struct gpio_t113_pdata_t *)chip->priv;
	u32_t val;

	if(offset >= chip->ngpio)
		return;

	val = read32(pdat->virt + GPIO_DAT);
	val &= ~(1 << offset);
	val |= (!!value) << offset;
	write32(pdat->virt + GPIO_DAT, val);
}

static int gpio_t113_get_value(struct gpiochip_t * chip, int offset)
{
	struct gpio_t113_pdata_t * pdat = (struct gpio_t113_pdata_t *)chip->priv;
	u32_t val;

	if(offset >= chip->ngpio)
		return 0;

	val = read32(pdat->virt + GPIO_DAT);
	return !!(val & (1 << offset));
}

static int gpio_t113_to_irq(struct gpiochip_t * chip, int offset)
{
	struct gpio_t113_pdata_t * pdat = (struct gpio_t113_pdata_t *)chip->priv;

	if((offset >= chip->ngpio) || (pdat->oirq < 0))
		return -1;
	return pdat->oirq + offset;
}
#endif
