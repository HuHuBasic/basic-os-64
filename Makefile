# ============================================================
# Basic OS 64-bit Makefile
# ============================================================

CC      = gcc
CFLAGS  = -m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
          -ffreestanding -O2 -Wall -Wextra \
          -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
          -fno-pic -fno-pie \
          -I./src -mcmodel=kernel $(EXTRA_CFLAGS)

AS      = as
ASFLAGS = --64

LD      = ld
LDFLAGS = -m elf_x86_64 -T linker64.ld -nostdlib

# Sources
ASM_SRCS = src/boot64.s
C_SRCS   = src/kernel.c \
           src/vga.c \
           src/gdt.c \
           src/idt.c \
           src/isr.c \
           src/irq.c \
           src/keyboard.c \
           src/timer.c \
           src/string.c \
           src/ports.c \
           src/bootscreen.c \
           src/heap.c \
           src/fs.c \
           src/app.c \
           src/pkg.c \
           src/settings.c \
           src/upgrade.c

ASM_OBJS = $(ASM_SRCS:.s=.o)
C_OBJS   = $(C_SRCS:.c=.o)
OBJS     = $(ASM_OBJS) $(C_OBJS)

TARGET   = kernel.bin
ISO      = basic-os-64.iso
ISO_DIR  = iso

.PHONY: all clean iso run run-qemu

all: $(TARGET)

# Link
$(TARGET): $(OBJS) linker64.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# Assemble
%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

# Compile
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Create bootable ISO
iso: $(TARGET)
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(TARGET) $(ISO_DIR)/boot/kernel.bin
	cp grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(ISO_DIR) 2>/dev/null || \
		xorriso -as mkisofs -b boot/grub/i386-pc/eltorito.img \
			-no-emul-boot -boot-load-size 4 -boot-info-table \
			-o $(ISO) $(ISO_DIR) 2>/dev/null || \
		grub2-mkrescue -o $(ISO) $(ISO_DIR) 2>/dev/null || \
		(echo "ERROR: Could not create ISO. Install grub-mkrescue or xorriso." && false)

# Run with QEMU
run: iso
	qemu-system-x86_64 -cdrom $(ISO) -m 128M -no-reboot -no-shutdown

# Clean
clean:
	rm -f $(OBJS) $(TARGET) $(ISO)
	rm -rf $(ISO_DIR)