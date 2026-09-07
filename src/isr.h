#ifndef ISR_H
#define ISR_H

#include "types.h"

/* Register state saved by ISR stub (System V AMD64 ABI order on stack) */
typedef struct {
    /* Segment registers (pushed by isr_common) */
    uint64_t ds;
    /* General-purpose registers (pushed by isr_common) */
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    /* Pushed by ISR macro */
    uint64_t int_no;
    uint64_t err_code;
    /* Pushed by CPU */
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) registers_t;

/* Exception names */
extern const char *exception_names[];

/* ISR handlers */
void isr_handler(registers_t *regs);

/* ISR stub declarations (defined in boot64.s) */
void isr0(void);
void isr1(void);
void isr2(void);
void isr3(void);
void isr4(void);
void isr5(void);
void isr6(void);
void isr7(void);
void isr8(void);
void isr9(void);
void isr10(void);
void isr11(void);
void isr12(void);
void isr13(void);
void isr14(void);
void isr15(void);
void isr16(void);
void isr17(void);
void isr18(void);
void isr19(void);
void isr20(void);
void isr21(void);
void isr22(void);
void isr23(void);
void isr24(void);
void isr25(void);
void isr26(void);
void isr27(void);
void isr28(void);
void isr29(void);
void isr30(void);
void isr31(void);

void isr_install(void);

#endif