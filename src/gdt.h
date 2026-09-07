#ifndef GDT_H
#define GDT_H

#include "types.h"

/* GDT entry structure (8 bytes) */
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

/* TSS entry structure (16 bytes = two 8-byte entries) */
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  flags_limit;
    uint8_t  base_high;
    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed)) gdt_tss_entry_t;

/* GDT descriptor for lgdt */
typedef struct {
    uint16_t size;
    uint64_t offset;
} __attribute__((packed)) gdt_descriptor_t;

/* TSS structure */
typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t io_map_base;
} __attribute__((packed)) tss_t;

/* GDT selectors */
#define GDT_NULL_SEL        0x00
#define GDT_KERNEL_CODE_SEL 0x08
#define GDT_KERNEL_DATA_SEL 0x10
#define GDT_USER_CODE_SEL   0x18
#define GDT_USER_DATA_SEL   0x20
#define GDT_KERNEL_CODE32_SEL 0x28
#define GDT_TSS_SEL         0x30

void gdt_init(void);
void gdt_flush(uint64_t gdt_desc);
void gdt_set_tss_stack(uint64_t rsp);

#endif