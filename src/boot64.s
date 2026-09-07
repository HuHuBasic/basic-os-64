# Multiboot2 constants
.set MULTIBOOT2_MAGIC,       0xE85250D6
.set MULTIBOOT2_ARCH,        0
.set MULTIBOOT2_HEADER_LEN,  multiboot_header_end - multiboot_header_start
.set MULTIBOOT2_CHECKSUM,    -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + MULTIBOOT2_HEADER_LEN)

# Page table flags
.set PT_PRESENT,  0x001
.set PT_WRITABLE, 0x002
.set PT_HUGE,     0x080   # 2MB page

# Higher half offset
.set HIGHER_HALF, 0xFFFFFFFF80000000

# ============================================================
# .multiboot section: Multiboot2 header + 32-bit entry + trampoline
# ============================================================
.section .multiboot, "ax"
.align 8

multiboot_header_start:
    .long MULTIBOOT2_MAGIC
    .long MULTIBOOT2_ARCH
    .long MULTIBOOT2_HEADER_LEN
    .long MULTIBOOT2_CHECKSUM

    # Framebuffer tag
    .align 8
    .short 5            # type: framebuffer
    .short 0            # flags
    .long 20            # size
    .long 0             # width
    .long 0             # height
    .long 0             # depth

    # End tag
    .align 8
    .short 0            # type: end
    .short 0            # flags
    .long 8             # size
multiboot_header_end:

# ============================================================
# 32-bit entry point (GRUB jumps here in protected mode)
# ============================================================
.code32
.global _start
.type _start, @function
_start:
    cli
    cld

    # Save multiboot2 info pointer (in EBX from GRUB)
    movl %ebx, multiboot_info_phys

    # ---------------------------------------------------------
    # Step 1: Check CPUID support
    # ---------------------------------------------------------
    pushfl
    popl %eax
    movl %eax, %ecx
    xorl $(1 << 21), %eax
    pushl %eax
    popfl
    pushfl
    popl %eax
    pushl %ecx
    popfl
    xorl %ecx, %eax
    testl $(1 << 21), %eax
    jz no_cpuid

    # ---------------------------------------------------------
    # Step 2: Check long mode support via CPUID
    # ---------------------------------------------------------
    movl $0x80000000, %eax
    cpuid
    cmpl $0x80000001, %eax
    jb no_long_mode

    movl $0x80000001, %eax
    cpuid
    testl $(1 << 29), %edx   # LM bit
    jz no_long_mode

    # ---------------------------------------------------------
    # Step 3: Clear BSS (using physical addresses from linker)
    # ---------------------------------------------------------
    movl $__bss_phys_start, %edi
    movl $__bss_phys_end, %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    rep stosb

    # ---------------------------------------------------------
    # Step 4: Set up page tables
    # ---------------------------------------------------------
    # PML4[0]     -> PDPT (identity mapping)
    # PML4[511]   -> PDPT (higher half mapping)
    # PDPT[0]     -> PD (identity)
    # PDPT[510]   -> PD (higher half)
    # PD[0]       -> 0x00000000 | 2MB page (maps first 2MB)

    # PML4[0] = PDPT physical | present | writable
    movl $pdpt_phys, %eax
    orl  $(PT_PRESENT | PT_WRITABLE), %eax
    movl %eax, pml4_phys

    # PML4[511] = PDPT physical | present | writable
    movl $pdpt_phys, %eax
    orl  $(PT_PRESENT | PT_WRITABLE), %eax
    movl %eax, pml4_phys + 511*8

    # PDPT[0] = PD physical | present | writable
    movl $pd_phys, %eax
    orl  $(PT_PRESENT | PT_WRITABLE), %eax
    movl %eax, pdpt_phys

    # PDPT[510] = PD physical | present | writable
    movl $pd_phys, %eax
    orl  $(PT_PRESENT | PT_WRITABLE), %eax
    movl %eax, pdpt_phys + 510*8

    # PD[0] = 0x00000000 | present | writable | huge (2MB page)
    movl $(PT_PRESENT | PT_WRITABLE | PT_HUGE), %eax
    movl %eax, pd_phys

    # ---------------------------------------------------------
    # Step 5: Enable PAE (CR4.PAE)
    # ---------------------------------------------------------
    movl %cr4, %eax
    orl  $(1 << 5), %eax
    movl %eax, %cr4

    # ---------------------------------------------------------
    # Step 6: Set EFER.LME = 1
    # ---------------------------------------------------------
    movl $0xC0000080, %ecx   # EFER MSR
    rdmsr
    orl  $(1 << 8), %eax     # LME bit
    wrmsr

    # ---------------------------------------------------------
    # Step 7: Load CR3 with PML4 physical address
    # ---------------------------------------------------------
    movl $pml4_phys, %eax
    movl %eax, %cr3

    # ---------------------------------------------------------
    # Step 8: Enable paging (CR0.PG = 1)
    # ---------------------------------------------------------
    movl %cr0, %eax
    orl  $(1 << 31), %eax
    movl %eax, %cr0

    # ---------------------------------------------------------
    # Step 9: Load 64-bit GDT (using physical address)
    # ---------------------------------------------------------
    lgdt gdt64_desc_phys

    # ---------------------------------------------------------
    # Step 10: Far jump to 64-bit trampoline
    # ---------------------------------------------------------
    ljmpl $0x08, $trampoline64_phys

# ============================================================
# Error handlers
# ============================================================
no_cpuid:
    movl $0x4F434F4E, 0xB8000
    movl $0x4F55504F, 0xB8004
    movl $0x4F44494F, 0xB8008
    hlt_loop_cpuid:
        hlt
        jmp hlt_loop_cpuid

no_long_mode:
    movl $0x4F4F4E4F, 0xB8000
    movl $0x4F4D474F, 0xB8004
    hlt_loop_lm:
        hlt
        jmp hlt_loop_lm

# ============================================================
# 64-bit trampoline (at low physical address)
# ============================================================
.code64
.align 8
trampoline64_phys:
    # Now in 64-bit long mode, running at identity-mapped address

    # Set up data segments
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # Jump to higher-half kernel code
    movabs $higher_half_entry, %rax
    jmp *%rax

# ============================================================
# .data section: temporary 64-bit GDT
# ============================================================
.section .data
.align 16
.global gdt64
gdt64:
    .quad 0                     # Null descriptor
    .quad 0x0020980000000000    # 64-bit code (selector 0x08)
    .quad 0x0000920000000000    # 64-bit data (selector 0x10)
gdt64_end:

.align 16
gdt64_desc:
    .word gdt64_end - gdt64 - 1
    .quad gdt64
gdt64_desc_phys = gdt64_desc - HIGHER_HALF

.align 8
.global multiboot_info_ptr
multiboot_info_ptr:
    .quad 0
multiboot_info_phys = multiboot_info_ptr - HIGHER_HALF

# ============================================================
# .bss section: page tables, stack
# ============================================================
.section .bss
.align 4096
.global pml4
pml4:
    .skip 4096
pml4_phys = pml4 - HIGHER_HALF

.align 4096
.global pdpt
pdpt:
    .skip 4096
pdpt_phys = pdpt - HIGHER_HALF

.align 4096
.global pd
pd:
    .skip 4096
pd_phys = pd - HIGHER_HALF

.align 16
.global stack_top
stack_bottom:
    .skip 16384   # 16KB stack
stack_top:
stack_top_phys = stack_top - HIGHER_HALF

# ============================================================
# .text section: 64-bit kernel code
# ============================================================
.section .text
.code64

higher_half_entry:
    # Now running at higher-half address

    # Reload GDT with higher-half addresses
    lgdt gdt64_desc(%rip)

    # Reload segment registers
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # Set up stack (higher-half address)
    movabs $stack_top, %rsp
    movq %rsp, %rbp

    # Clear direction flag
    cld

    # Prepare multiboot2 info pointer (convert to higher-half)
    movq multiboot_info_ptr(%rip), %rdi
    testq %rdi, %rdi
    jz 1f
    movabs $HIGHER_HALF, %rax
    addq %rax, %rdi
1:

    # Call kernel_main
    call kernel_main

    # Halt if kernel_main returns
halt_loop:
    hlt
    jmp halt_loop

# ============================================================
# ISR common handler
# ============================================================
.global isr_common
.type isr_common, @function
isr_common:
    # Save all registers (System V AMD64)
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # Save DS
    movq %ds, %rax
    pushq %rax

    # Set kernel data segments
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Pass pointer to saved registers as first argument (RDI)
    movq %rsp, %rdi

    # Call C handler
    call isr_handler

    # Restore DS
    popq %rax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Restore registers
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax

    # Remove error code and interrupt number
    addq $16, %rsp

    iretq

# ============================================================
# IRQ common handler
# ============================================================
.global irq_common
.type irq_common, @function
irq_common:
    # Save all registers
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # Save DS
    movq %ds, %rax
    pushq %rax

    # Set kernel data segments
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Pass pointer to saved registers
    movq %rsp, %rdi

    # Call C handler
    call irq_handler

    # Restore DS
    popq %rax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Restore registers
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax

    # Remove error code and interrupt number
    addq $16, %rsp

    iretq

# ============================================================
# ISR stub macros
# ============================================================
.macro ISR_NOERR n
.global isr\n
.type isr\n, @function
isr\n:
    pushq $0            # dummy error code
    pushq $\n           # interrupt number
    jmp isr_common
.endm

.macro ISR_ERR n
.global isr\n
.type isr\n, @function
isr\n:
    pushq $\n           # interrupt number (error code already pushed)
    jmp isr_common
.endm

# ============================================================
# ISR stubs (0-31)
# ============================================================
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR    8
ISR_NOERR 9
ISR_ERR    10
ISR_ERR    11
ISR_ERR    12
ISR_ERR    13
ISR_ERR    14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR    17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

# ============================================================
# IRQ stub macro
# ============================================================
.macro IRQ n, intno
.global irq\n
.type irq\n, @function
irq\n:
    pushq $0            # dummy error code
    pushq $\intno       # interrupt number
    jmp irq_common
.endm

# ============================================================
# IRQ stubs (0-15)
# ============================================================
IRQ 0,  32
IRQ 1,  33
IRQ 2,  34
IRQ 3,  35
IRQ 4,  36
IRQ 5,  37
IRQ 6,  38
IRQ 7,  39
IRQ 8,  40
IRQ 9,  41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47
