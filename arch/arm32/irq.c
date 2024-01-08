/*
 * exception.c
 *
 * Copyright(c) 2007-2022 Jianjun Jiang <8192542@qq.com>
 * Official site: http://xboot.org
 * Mobile phone: +86-18665388956
 * QQ: 8192542
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <arm32.h>
#include "main.h"
#include "debug.h"
#include "irq.h"
#include "sunxi_irq.h"

enum {
	DIST_CTRL		   = 0x1000,
	DIST_CTR		   = 0x1004,
	DIST_ENABLE_SET	   = 0x1100,
	DIST_ENABLE_CLEAR  = 0x1180,
	DIST_PENDING_SET   = 0x1200,
	DIST_PENDING_CLEAR = 0x1280,
	DIST_ACTIVE_BIT	   = 0x1300,
	DIST_PRI		   = 0x1400,
	DIST_TARGET		   = 0x1800,
	DIST_CONFIG		   = 0x1c00,
	DIST_SOFTINT	   = 0x1f00,

	CPU_CTRL	   = 0x2000,
	CPU_PRIMASK	   = 0x2004,
	CPU_BINPOINT   = 0x2008,
	CPU_INTACK	   = 0x200c,
	CPU_EOI		   = 0x2010,
	CPU_RUNNINGPRI = 0x2014,
	CPU_HIGHPRI	   = 0x2018,
};

struct irq_tbl_t {
	irq_handler_t handler;
	void		 *arg;
};

struct irq_tbl_t irq_table[IRQ_MAX];

struct arm_regs_t {
	uint32_t esp;
	uint32_t cpsr;
	uint32_t r[13];
	uint32_t sp;
	uint32_t lr;
	uint32_t pc;
};

static void default_handler(void *arg);

static void show_regs(struct arm_regs_t *regs)
{
	int i;

	debug("pc : [<%08lx>] lr : [<%08lx>] cpsr: %08lx\r\n", regs->pc, regs->lr, regs->cpsr);
	debug("sp : %08lx esp : %08lx\r\n", regs->sp, regs->esp);
	for (i = 12; i >= 0; i--)
		debug("r%-2d: %08lx\r\n", i, regs->r[i]);
	debug("\r\n");
}

void arm32_do_undefined_instruction(struct arm_regs_t *regs)
{
	show_regs(regs);
	debug("%s\r\n", __FUNCTION__);
	regs->pc += 4;
}

void arm32_do_software_interrupt(struct arm_regs_t *regs)
{
	show_regs(regs);
	debug("%s\r\n", __FUNCTION__);
	regs->pc += 4;
}

void arm32_do_prefetch_abort(struct arm_regs_t *regs)
{
	show_regs(regs);
	debug("%s\r\n", __FUNCTION__);
	regs->pc += 4;
}

void arm32_do_data_abort(struct arm_regs_t *regs)
{
	show_regs(regs);
	debug("%s\r\n", __FUNCTION__);
	regs->pc += 4;
}

void arm32_do_irq(struct arm_regs_t *regs)
{
	(void)regs;

	int irq = read32(GIC400_BASE_ADDR + CPU_INTACK) & 0x3ff;

	if (irq < ARRAY_SIZE(irq_table)) {
		if (irq_table[irq].handler)
			irq_table[irq].handler(irq_table[irq].arg);
	}
	write32(GIC400_BASE_ADDR + CPU_EOI, irq);
}

void arm32_do_fiq(struct arm_regs_t *regs)
{
	(void)regs;

	int irq = read32(GIC400_BASE_ADDR + CPU_INTACK) & 0x3ff;

	if (irq < ARRAY_SIZE(irq_table)) {
		if (irq_table[irq].handler)
			irq_table[irq].handler(irq_table[irq].arg);
	}
	write32(GIC400_BASE_ADDR + CPU_EOI, irq);
}

void default_handler(void *arg)
{
	(void)arg;

	debug("irq: unexpected irq \r\n");
	while (1)
		;
}

void gic400_irq_enable(int irq)
{
	write32(GIC400_BASE_ADDR + DIST_ENABLE_SET + (irq / 32) * 4, 1 << (irq % 32));
}

void gic400_irq_disable(int irq)
{
	write32(GIC400_BASE_ADDR + DIST_ENABLE_CLEAR + (irq / 32) * 4, 1 << (irq % 32));
}

static void gic400_dist_init()
{
	uint32_t gic_irqs;
	uint32_t cpumask;
	int		 i;

	write32(GIC400_BASE_ADDR + DIST_CTRL, 0x0);

	/*
	 * Find out how many interrupts are supported.
	 * The GIC only supports up to 1020 interrupt sources.
	 */
	gic_irqs = read32(GIC400_BASE_ADDR + DIST_CTR) & 0x1f;
	gic_irqs = (gic_irqs + 1) * 32;
	if (gic_irqs > 1020)
		gic_irqs = 1020;

	/*
	 * Set all global interrupts to this CPU only.
	 */
	cpumask = 1 << 0;
	cpumask |= cpumask << 8;
	cpumask |= cpumask << 16;
	for (i = 32; i < gic_irqs; i += 4)
		write32(GIC400_BASE_ADDR + DIST_TARGET + i * 4 / 4, cpumask);

	/*
	 * Set all global interrupts to be level triggered, active low.
	 */
	for (i = 32; i < gic_irqs; i += 16)
		write32(GIC400_BASE_ADDR + DIST_CONFIG + i * 4 / 16, 0);

	/*
	 * Set priority on all global interrupts.
	 */
	for (i = 32; i < gic_irqs; i += 4)
		write32(GIC400_BASE_ADDR + DIST_PRI + i * 4 / 4, 0xa0a0a0a0);

	/*
	 * Disable all interrupts, leave the SGI and PPI alone
	 * as these enables are banked registers.
	 */
	for (i = 32; i < gic_irqs; i += 32)
		write32(GIC400_BASE_ADDR + DIST_ENABLE_CLEAR + i * 4 / 32, 0xffffffff);

	write32(GIC400_BASE_ADDR + DIST_CTRL, 0x1);
}

static void gic400_cpu_init()
{
	int i;

	/*
	 * Deal with the banked SGI and PPI interrupts - enable all
	 * SGI interrupts, ensure all PPI interrupts are disabled.
	 */
	write32(GIC400_BASE_ADDR + DIST_ENABLE_CLEAR, 0xffff0000);
	write32(GIC400_BASE_ADDR + DIST_ENABLE_SET, 0x0000ffff);

	/*
	 * Set priority on SGI and PPI interrupts
	 */
	for (i = 0; i < 32; i += 4)
		write32(GIC400_BASE_ADDR + DIST_PRI + i * 4 / 4, 0xa0a0a0a0);

	write32(GIC400_BASE_ADDR + CPU_PRIMASK, 0xf0);
	write32(GIC400_BASE_ADDR + CPU_CTRL, 0x1);
}

void gic400_set_irq_handler(uint32_t irq, irq_handler_t handler, void *arg)
{
	irq_table[irq].handler = handler;
	irq_table[irq].arg	   = arg;
}

void gic400_init()
{
	// set default irq handler
	for (uint32_t i = 0; i < IRQ_MAX; i++) {
		irq_table[i].handler = default_handler;
		irq_table[i].arg	 = 0;
	}

	gic400_dist_init();
	gic400_cpu_init();

	arm32_interrupt_enable();
}
