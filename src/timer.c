#include "timer.h"
#include "ports.h"
#include "irq.h"

#define PIT_CHANNEL0  0x40
#define PIT_CHANNEL1  0x41
#define PIT_CHANNEL2  0x42
#define PIT_COMMAND   0x43

/* PIT base frequency: 1193180 Hz */
#define PIT_FREQUENCY  1193180

/* Target frequency: 100 Hz */
#define TARGET_FREQ    100

static volatile uint64_t tick_count = 0;

void timer_handler(registers_t *regs)
{
    (void)regs;
    tick_count++;
}

uint64_t timer_get_ticks(void)
{
    return tick_count;
}

void timer_sleep(uint64_t ms)
{
    uint64_t target = tick_count + (ms * TARGET_FREQ) / 1000;
    while (tick_count < target) {
        __asm__ volatile ("hlt");
    }
}

void timer_init(void)
{
    tick_count = 0;

    uint32_t divisor = PIT_FREQUENCY / TARGET_FREQ;

    /* Set PIT to mode 2 (rate generator) */
    outb(PIT_COMMAND, 0x34);  /* Channel 0, lobyte/hibyte, mode 2, binary */

    /* Send divisor */
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    irq_register_handler(0, timer_handler);
}