#include "gdt.h"
#include "string.h"

/* Number of GDT entries: null, kcode64, kdata, ucode64, udata, kcode32, TSS(low+high) = 8 */
#define GDT_ENTRIES 8

static gdt_entry_t     gdt[GDT_ENTRIES];
static gdt_tss_entry_t gdt_tss;
static tss_t           tss;
static gdt_descriptor_t gdt_desc;

static void gdt_set_entry(int idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity)
{
    gdt[idx].limit_low    = (uint16_t)(limit & 0xFFFF);
    gdt[idx].base_low     = (uint16_t)(base & 0xFFFF);
    gdt[idx].base_mid     = (uint8_t)((base >> 16) & 0xFF);
    gdt[idx].access       = access;
    gdt[idx].granularity  = (uint8_t)(((limit >> 16) & 0x0F) | (granularity & 0xF0));
    gdt[idx].base_high    = (uint8_t)((base >> 24) & 0xFF);
}

static void gdt_set_tss_entry(uint64_t base, uint32_t limit)
{
    gdt_tss.limit_low   = (uint16_t)(limit & 0xFFFF);
    gdt_tss.base_low    = (uint16_t)(base & 0xFFFF);
    gdt_tss.base_mid    = (uint8_t)((base >> 16) & 0xFF);
    gdt_tss.access      = 0x89;   /* Present, DPL=0, 64-bit TSS available */
    gdt_tss.flags_limit = (uint8_t)(((limit >> 16) & 0x0F) | 0x00);
    gdt_tss.base_high   = (uint8_t)((base >> 24) & 0xFF);
    gdt_tss.base_upper  = (uint32_t)(base >> 32);
    gdt_tss.reserved    = 0;
}

void gdt_init(void)
{
    /* Clear GDT */
    memset(gdt, 0, sizeof(gdt));
    memset(&gdt_tss, 0, sizeof(gdt_tss));
    memset(&tss, 0, sizeof(tss));

    /* Entry 0: Null descriptor */
    gdt_set_entry(0, 0, 0, 0, 0);

    /* Entry 1: Kernel Code 64-bit
     * Access: Present=1, DPL=0, S=1, Executable=1, DC=0, RW=1, Accessed=0
     *          = 0x9A
     * Granularity: G=0, D=0, L=1, AVL=0
     *          = 0x20 (Long mode)
     */
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0x20);

    /* Entry 2: Kernel Data
     * Access: Present=1, DPL=0, S=1, Executable=0, DC=0, RW=1, Accessed=0
     *          = 0x92
     * Granularity: G=0, D=0, L=0, AVL=0
     */
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0x00);

    /* Entry 3: User Code 64-bit
     * Access: Present=1, DPL=3, S=1, Executable=1, DC=0, RW=1, Accessed=0
     *          = 0xFA
     * Granularity: G=0, D=0, L=1, AVL=0
     */
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA, 0x20);

    /* Entry 4: User Data
     * Access: Present=1, DPL=3, S=1, Executable=0, DC=0, RW=1, Accessed=0
     *          = 0xF2
     */
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2, 0x00);

    /* Entry 5: Kernel Code 32-bit (compatibility mode)
     * Access: Present=1, DPL=0, S=1, Executable=1, DC=0, RW=1, Accessed=0
     *          = 0x9A
     * Granularity: G=0, D=1, L=0, AVL=0
     */
    gdt_set_entry(5, 0, 0xFFFFF, 0x9A, 0x40);

    /* Entry 6-7: TSS (16 bytes, spanning two GDT entries) */
    gdt_set_tss_entry((uint64_t)&tss, sizeof(tss_t) - 1);

    /* Setup GDT descriptor */
    gdt_desc.size = (uint16_t)(sizeof(gdt) + sizeof(gdt_tss) - 1);
    gdt_desc.offset = (uint64_t)&gdt;

    /* Load GDT */
    __asm__ volatile (
        "lgdt (%0)\n"
        :
        : "r"(&gdt_desc)
        : "memory"
    );

    /* Reload segment registers */
    __asm__ volatile (
        "movw %w0, %%ds\n"
        "movw %w0, %%es\n"
        "movw %w0, %%fs\n"
        "movw %w0, %%gs\n"
        "movw %w0, %%ss\n"
        /* Far return to reload CS */
        "pushq %1\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        :
        : "r"((uint64_t)GDT_KERNEL_DATA_SEL), "r"((uint64_t)GDT_KERNEL_CODE_SEL)
        : "rax", "memory"
    );

    /* Load TSS */
    __asm__ volatile (
        "ltr %%ax\n"
        :
        : "a"(GDT_TSS_SEL)
    );
}

void gdt_set_tss_stack(uint64_t rsp)
{
    tss.rsp0 = rsp;
}