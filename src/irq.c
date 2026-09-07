#include "irq.h"
#include "idt.h"
#include "gdt.h"
#include "ports.h"
#include "vga.h"

#define PIC1            0x20    /* Master PIC */
#define PIC2            0xA0    /* Slave PIC */
#define PIC1_COMMAND    PIC1
#define PIC1_DATA       (PIC1 + 1)
#define PIC2_COMMAND    PIC2
#define PIC2_DATA       (PIC2 + 1)

#define PIC_EOI         0x20    /* End-of-interrupt command */

/* Remapped IRQ offsets */
#define IRQ_OFFSET_MASTER  0x20
#define IRQ_OFFSET_SLAVE   0x28

static irq_handler_t irq_handlers[16];

void irq_handler(registers_t *regs)
{
    int irq = regs->int_no - IRQ_OFFSET_MASTER;

    /* Send EOI to slave PIC if IRQ came from slave */
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }

    /* Send EOI to master PIC */
    outb(PIC1_COMMAND, PIC_EOI);

    /* Call registered handler */
    if (irq >= 0 && irq < 16 && irq_handlers[irq] != NULL) {
        irq_handlers[irq](regs);
    }
}

void irq_register_handler(int irq, irq_handler_t handler)
{
    if (irq >= 0 && irq < 16) {
        irq_handlers[irq] = handler;
    }
}

void irq_init(void)
{
    int i;
    for (i = 0; i < 16; i++) {
        irq_handlers[i] = NULL;
    }

    /* Remap PICs:
     * Start initialization (ICW1): IC4 needed, cascade mode
     */
    outb(PIC1_COMMAND, 0x11);
    io_wait();
    outb(PIC2_COMMAND, 0x11);
    io_wait();

    /* ICW2: Set vector offsets */
    outb(PIC1_DATA, IRQ_OFFSET_MASTER);  /* Master: IRQ0-7 -> INT 0x20-0x27 */
    io_wait();
    outb(PIC2_DATA, IRQ_OFFSET_SLAVE);   /* Slave: IRQ8-15 -> INT 0x28-0x2F */
    io_wait();

    /* ICW3: Cascade configuration */
    outb(PIC1_DATA, 0x04);  /* Master has slave on IRQ2 (bit 2) */
    io_wait();
    outb(PIC2_DATA, 0x02);  /* Slave identity is 2 */
    io_wait();

    /* ICW4: 8086/88 mode */
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    /* Mask all interrupts initially */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void irq_install(void)
{
    /* Install IRQ handlers in IDT */
    idt_set_entry(IRQ_OFFSET_MASTER + 0,  (uint64_t)irq0,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(IRQ_OFFSET_MASTER + 1,  (uint64_t)irq1,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(IRQ_OFFSET_MASTER + 2,  (uint64_t)irq2,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(IRQ_OFFSET_MASTER + 3,  (uint64_t)irq3,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(IRQ_OFFSET_MASTER + 4,  (uint64_t)irq4,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(IRQ_OFFSET_MASTER + 5,  (uint64_t)irq5,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(IRQ_OFFSET_MASTER + 6,  (uint64_t)irq6,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(IRQ_OFFSET_MASTER + 7,  (uint64_t)irq7,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(IRQ_OFFSET_SLAVE + 0,   (uint64_t)irq8,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(IRQ_OFFSET_SLAVE + 1,   (uint64_t)irq9,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(IRQ_OFFSET_SLAVE + 2,   (uint64_t)irq10, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(IRQ_OFFSET_SLAVE + 3,   (uint64_t)irq11, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(IRQ_OFFSET_SLAVE + 4,   (uint64_t)irq12, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(IRQ_OFFSET_SLAVE + 5,   (uint64_t)irq13, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(IRQ_OFFSET_SLAVE + 6,   (uint64_t)irq14, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(IRQ_OFFSET_SLAVE + 7,   (uint64_t)irq15, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
}