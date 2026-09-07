#include "isr.h"
#include "idt.h"
#include "vga.h"
#include "gdt.h"

const char *exception_names[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void isr_handler(registers_t *regs)
{
    terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    terminal_print("\n[EXCEPTION] ");
    if (regs->int_no < 32) {
        terminal_print(exception_names[regs->int_no]);
    } else {
        terminal_print("Unknown");
    }
    terminal_print(" (int=");
    terminal_print_dec(regs->int_no);
    terminal_print(", err=");
    terminal_print_hex(regs->err_code);
    terminal_print(")\n");

    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_print("  RIP=");
    terminal_print_hex(regs->rip);
    terminal_print(" CS=");
    terminal_print_hex(regs->cs);
    terminal_print(" RFLAGS=");
    terminal_print_hex(regs->rflags);
    terminal_print("\n");
    terminal_print("  RSP=");
    terminal_print_hex(regs->rsp);
    terminal_print(" SS=");
    terminal_print_hex(regs->ss);
    terminal_print("\n");
    terminal_print("  RAX=");
    terminal_print_hex(regs->rax);
    terminal_print(" RBX=");
    terminal_print_hex(regs->rbx);
    terminal_print("\n");
    terminal_print("  RCX=");
    terminal_print_hex(regs->rcx);
    terminal_print(" RDX=");
    terminal_print_hex(regs->rdx);
    terminal_print("\n");
    terminal_print("  RSI=");
    terminal_print_hex(regs->rsi);
    terminal_print(" RDI=");
    terminal_print_hex(regs->rdi);
    terminal_print("\n");
    terminal_print("  RBP=");
    terminal_print_hex(regs->rbp);
    terminal_print(" R8 =");
    terminal_print_hex(regs->r8);
    terminal_print("\n");
    terminal_print("  R9 =");
    terminal_print_hex(regs->r9);
    terminal_print(" R10=");
    terminal_print_hex(regs->r10);
    terminal_print("\n");
    terminal_print("  R11=");
    terminal_print_hex(regs->r11);
    terminal_print(" R12=");
    terminal_print_hex(regs->r12);
    terminal_print("\n");
    terminal_print("  R13=");
    terminal_print_hex(regs->r13);
    terminal_print(" R14=");
    terminal_print_hex(regs->r14);
    terminal_print("\n");
    terminal_print("  R15=");
    terminal_print_hex(regs->r15);
    terminal_print("\n");

    /* Halt the CPU */
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    terminal_print("\n  SYSTEM HALTED.\n");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}

void isr_install(void)
{
    /* Install ISR handlers for exceptions 0-31 */
    idt_set_entry(0,  (uint64_t)isr0,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(1,  (uint64_t)isr1,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(2,  (uint64_t)isr2,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(3,  (uint64_t)isr3,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(4,  (uint64_t)isr4,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(5,  (uint64_t)isr5,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(6,  (uint64_t)isr6,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(7,  (uint64_t)isr7,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(8,  (uint64_t)isr8,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(9,  (uint64_t)isr9,  GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(10, (uint64_t)isr10, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(11, (uint64_t)isr11, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(12, (uint64_t)isr12, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(13, (uint64_t)isr13, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(14, (uint64_t)isr14, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(15, (uint64_t)isr15, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(16, (uint64_t)isr16, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(17, (uint64_t)isr17, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(18, (uint64_t)isr18, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(19, (uint64_t)isr19, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(20, (uint64_t)isr20, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(21, (uint64_t)isr21, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(22, (uint64_t)isr22, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(23, (uint64_t)isr23, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(24, (uint64_t)isr24, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(25, (uint64_t)isr25, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(26, (uint64_t)isr26, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(27, (uint64_t)isr27, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(28, (uint64_t)isr28, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(29, (uint64_t)isr29, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(30, (uint64_t)isr30, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
    idt_set_entry(31, (uint64_t)isr31, GDT_KERNEL_CODE_SEL, 0, IDT_GATE_INTERRUPT);
}