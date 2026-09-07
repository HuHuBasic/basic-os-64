#ifndef IRQ_H
#define IRQ_H

#include "types.h"
#include "isr.h"

/* IRQ handlers */
void irq0(void);
void irq1(void);
void irq2(void);
void irq3(void);
void irq4(void);
void irq5(void);
void irq6(void);
void irq7(void);
void irq8(void);
void irq9(void);
void irq10(void);
void irq11(void);
void irq12(void);
void irq13(void);
void irq14(void);
void irq15(void);

void irq_handler(registers_t *regs);
void irq_init(void);
void irq_install(void);

/* Callback types for IRQ handlers */
typedef void (*irq_handler_t)(registers_t *regs);

void irq_register_handler(int irq, irq_handler_t handler);

#endif