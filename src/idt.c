#include "idt.h"
#include "string.h"

static idt_entry_t      idt[IDT_ENTRIES];
static idt_descriptor_t idt_desc;

void idt_set_entry(int idx, uint64_t handler, uint16_t selector, uint8_t ist, uint8_t type_attr)
{
    if (idx < 0 || idx >= IDT_ENTRIES)
        return;

    idt[idx].offset_low  = (uint16_t)(handler & 0xFFFF);
    idt[idx].selector    = selector;
    idt[idx].ist         = ist & 0x07;
    idt[idx].type_attr   = type_attr;
    idt[idx].offset_mid  = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[idx].offset_high = (uint32_t)((handler >> 32) & 0xFFFFFFFF);
    idt[idx].reserved    = 0;
}

void idt_init(void)
{
    /* Clear IDT */
    memset(idt, 0, sizeof(idt));

    /* Setup IDT descriptor */
    idt_desc.size   = (uint16_t)(sizeof(idt) - 1);
    idt_desc.offset = (uint64_t)&idt;

    /* Load IDT */
    __asm__ volatile (
        "lidt (%0)\n"
        :
        : "r"(&idt_desc)
        : "memory"
    );
}