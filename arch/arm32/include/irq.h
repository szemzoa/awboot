#ifndef __IRQ_H__
#define __IRQ_H__

typedef void (*irq_handler_t)(void *arg);

extern void gic400_init();
extern void gic400_set_irq_handler(uint32_t irq, irq_handler_t handler, void *arg);
extern void gic400_irq_enable(int irq);
extern void gic400_irq_disable(int irq);

#endif
