#ifndef IDT_H
#define IDT_H

#include "types.h"

/* IDT entry: 64-bit interrupt gate */
typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;          /* Interrupt Stack Table index (0-7) */
    uint8_t  type_attr;    /* Type and attributes */
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

/* IDT descriptor for lidt */
typedef struct {
    uint16_t size;
    uint64_t offset;
} __attribute__((packed)) idt_descriptor_t;

#define IDT_ENTRIES 256

/* Interrupt gate types */
#define IDT_GATE_INTERRUPT 0x8E  /* Present, DPL=0, 64-bit interrupt gate */
#define IDT_GATE_TRAP      0x8F  /* Present, DPL=0, 64-bit trap gate */

void idt_init(void);
void idt_set_entry(int idx, uint64_t handler, uint16_t selector, uint8_t ist, uint8_t type_attr);

#endif