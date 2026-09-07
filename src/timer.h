#ifndef TIMER_H
#define TIMER_H

#include "types.h"
#include "isr.h"

void timer_init(void);
void timer_handler(registers_t *regs);
uint64_t timer_get_ticks(void);
void timer_sleep(uint64_t ms);

#endif