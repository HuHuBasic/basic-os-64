OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
    /* Start at 1MB physical */
    . = 1M;

    /* Multiboot2 header + 32-bit entry + trampoline (low physical) */
    .multiboot : {
        *(.multiboot)
    }

    . = ALIGN(4K);

    /* Higher half kernel offset */
    . += 0xFFFFFFFF80000000;

    /* Code */
    .text : AT(ADDR(.text) - 0xFFFFFFFF80000000) {
        *(.text)
        *(.text.*)
    }

    . = ALIGN(4K);

    /* Read-only data */
    .rodata : AT(ADDR(.rodata) - 0xFFFFFFFF80000000) {
        *(.rodata)
        *(.rodata.*)
    }

    . = ALIGN(4K);

    /* Initialized data */
    .data : AT(ADDR(.data) - 0xFFFFFFFF80000000) {
        *(.data)
        *(.data.*)
    }

    . = ALIGN(4K);

    /* BSS (uninitialized data) */
    __bss_start = .;
    .bss : AT(ADDR(.bss) - 0xFFFFFFFF80000000) {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }
    . = ALIGN(4K);
    __bss_end = .;

    /* Physical addresses for BSS (used by 32-bit boot code) */
    __bss_phys_start = __bss_start - 0xFFFFFFFF80000000;
    __bss_phys_end   = __bss_end   - 0xFFFFFFFF80000000;

    /* End of kernel */
    _kernel_end = .;
    _kernel_phys_end = _kernel_end - 0xFFFFFFFF80000000;
}