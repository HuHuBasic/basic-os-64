#include "types.h"
#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "keyboard.h"
#include "timer.h"
#include "string.h"
#include "ports.h"
#include "bootscreen.h"
#include "heap.h"
#include "fs.h"
#include "app.h"
#include "pkg.h"
#include "settings.h"
#include "upgrade.h"

/* Multiboot2 information structures */
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_tag_basic_meminfo {
    uint32_t type;
    uint32_t size;
    uint32_t mem_lower;
    uint32_t mem_upper;
};

struct multiboot_tag_bootdev {
    uint32_t type;
    uint32_t size;
    uint32_t biosdev;
    uint32_t partition;
    uint32_t sub_parition;
};

struct multiboot_tag_cmdline {
    uint32_t type;
    uint32_t size;
    char string[0];
};

struct multiboot_tag_module {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char cmdline[0];
};

struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    /* entries follow */
};

struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t reserved;
};

struct multiboot_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    uint8_t  reserved;
    /* color info follows */
};

/* Multiboot2 fixed header */
struct multiboot_info {
    uint32_t total_size;
    uint32_t reserved;
    /* tags follow */
};

static void process_multiboot2(struct multiboot_info *mbi)
{
    terminal_print("Multiboot2 info at: ");
    terminal_print_hex((uint64_t)mbi);
    terminal_print("\n");

    if (mbi == NULL) {
        terminal_print("  No multiboot info provided.\n");
        return;
    }

    uint32_t total = mbi->total_size;
    struct multiboot_tag *tag = (struct multiboot_tag *)((uint64_t)mbi + 8);

    while (tag->type != 0 && (uint64_t)tag < (uint64_t)mbi + total) {
        switch (tag->type) {
        case 4: { /* Basic memory info */
            struct multiboot_tag_basic_meminfo *mem = (struct multiboot_tag_basic_meminfo *)tag;
            terminal_print("  Memory: lower=");
            terminal_print_dec(mem->mem_lower);
            terminal_print("KB, upper=");
            terminal_print_dec(mem->mem_upper);
            terminal_print("KB\n");
            break;
        }
        case 5: { /* Boot device */
            struct multiboot_tag_bootdev *bootdev = (struct multiboot_tag_bootdev *)tag;
            terminal_print("  Boot device: 0x");
            terminal_print_hex(bootdev->biosdev);
            terminal_print("\n");
            break;
        }
        case 1: { /* Command line */
            struct multiboot_tag_cmdline *cmd = (struct multiboot_tag_cmdline *)tag;
            terminal_print("  Cmdline: ");
            terminal_print(cmd->string);
            terminal_print("\n");
            break;
        }
        case 6: { /* Memory map */
            struct multiboot_tag_mmap *mmap = (struct multiboot_tag_mmap *)tag;
            terminal_print("  Memory map: ");
            terminal_print_dec((mmap->size - 16) / mmap->entry_size);
            terminal_print(" entries\n");
            break;
        }
        case 8: { /* Framebuffer */
            struct multiboot_tag_framebuffer *fb = (struct multiboot_tag_framebuffer *)tag;
            terminal_print("  Framebuffer: ");
            terminal_print_dec(fb->framebuffer_width);
            terminal_print("x");
            terminal_print_dec(fb->framebuffer_height);
            terminal_print("x");
            terminal_print_dec(fb->framebuffer_bpp);
            terminal_print(", addr=0x");
            terminal_print_hex(fb->framebuffer_addr);
            terminal_print("\n");
            break;
        }
        default:
            break;
        }

        /* Align tag size to 8 bytes */
        uint32_t tag_size = tag->size;
        if (tag_size & 7) {
            tag_size += 8 - (tag_size & 7);
        }
        tag = (struct multiboot_tag *)((uint64_t)tag + tag_size);
    }
}

/* Enable interrupts */
static inline void sti(void)
{
    __asm__ volatile ("sti");
}

/* Nano delay (busy loop) */
static void delay(int count)
{
    for (volatile int i = 0; i < count; i++) {
        __asm__ volatile ("nop");
    }
}

static void print_prompt(void)
{
    terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_print("basic64> ");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

/* strncmp: compare first n characters */
static int strncmp(const char *a, const char *b, uint64_t n)
{
    for (uint64_t i = 0; i < n; i++) {
        if (a[i] != b[i])
            return 1;
        if (a[i] == '\0')
            return 0;
    }
    return 0;
}

static void handle_command(const char *cmd)
{
    if (strcmp(cmd, "help") == 0) {
        terminal_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
        terminal_print("Available commands:\n");
        terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        terminal_print("  help   - Show this help message\n");
        terminal_print("  clear  - Clear the screen\n");
        terminal_print("  info   - Show system information\n");
        terminal_print("  time   - Show system uptime (ticks)\n");
        terminal_print("  echo   - Echo text back\n");
        terminal_print("  colors - Show all VGA colors\n");
        terminal_print("  apps   - List installed .BAK applications\n");
        terminal_print("  run    - Run an application: run <name>\n");
        terminal_print("  pkg    - List installed .BAS packages\n");
        terminal_print("  pkg uninstall <name> - Remove a package\n");
        terminal_print("  settings [get|set|reset] - Manage system settings\n");
        terminal_print("  upgrade [apply|rollback] - System update\n");
        terminal_print("  halt   - Halt the CPU\n");
    } else if (strcmp(cmd, "clear") == 0) {
        terminal_clear();
    } else if (strcmp(cmd, "info") == 0) {
        terminal_print("Basic OS 64-bit\n");
        terminal_print("Architecture: x86_64 (Long Mode)\n");
        terminal_print("Higher half kernel: 0xFFFFFFFF80000000\n");
        terminal_print("VGA: 80x25 text mode\n");
        terminal_print("Timer: PIT at 100Hz\n");
        terminal_print("Keyboard: PS/2 US QWERTY\n");
    } else if (strcmp(cmd, "time") == 0) {
        terminal_print("System ticks: ");
        terminal_print_dec(timer_get_ticks());
        terminal_print(" (");
        terminal_print_dec(timer_get_ticks() / 100);
        terminal_print(" seconds)\n");
    } else if (strcmp(cmd, "colors") == 0) {
        for (int i = 0; i < 16; i++) {
            terminal_set_color(i, VGA_COLOR_BLACK);
            terminal_print("  ");
            terminal_print_hex(i);
            terminal_print(" ");
            uint8_t bg = i;
            terminal_putchar_at(' ', 0, 0, (i << 4) | bg); /* dummy */
        }
        terminal_print("\n");
        terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        for (int i = 0; i < 16; i++) {
            terminal_set_color(i, VGA_COLOR_BLACK);
            terminal_print("  #");
            terminal_print_dec(i);
            terminal_print("  ");
            terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        }
        terminal_print("\n");
    } else if (strcmp(cmd, "halt") == 0) {
        terminal_print("Halting CPU...\n");
        for (;;) {
            __asm__ volatile ("hlt");
        }
    } else if (strcmp(cmd, "apps") == 0) {
        app_list();
    } else if (strncmp(cmd, "run ", 4) == 0) {
        app_run(cmd + 4);
    } else if (strcmp(cmd, "pkg") == 0) {
        pkg_list();
    } else if (strcmp(cmd, "pkg reinstall") == 0) {
        pkg_ensure_defaults();
        terminal_print("pkg: system package ready.\n");
    } else if (strncmp(cmd, "pkg uninstall ", 14) == 0) {
        pkg_uninstall(cmd + 14);
    } else if (strcmp(cmd, "settings") == 0) {
        settings_list_print();
    } else if (strcmp(cmd, "settings reset") == 0) {
        settings_reset();
    } else if (strncmp(cmd, "settings get ", 13) == 0) {
        char value[128];
        if (settings_get(cmd + 13, value, sizeof(value)) == 0) {
            terminal_print(cmd + 13);
            terminal_print("=");
            terminal_print(value);
            terminal_print("\n");
        } else {
            terminal_print("settings: key not found: ");
            terminal_print(cmd + 13);
            terminal_print("\n");
        }
    } else if (strncmp(cmd, "settings set ", 13) == 0) {
        const char *p = cmd + 13;
        char key[64];
        uint32_t k = 0;
        while (*p && *p != ' ' && k < sizeof(key) - 1)
            key[k++] = *p++;
        key[k] = '\0';
        while (*p == ' ') p++;
        if (k > 0 && settings_set(key, p) == 0) {
            terminal_print("settings: ");
            terminal_print(key);
            terminal_print("=");
            terminal_print(p);
            terminal_print(" saved\n");
        } else {
            terminal_print("settings: invalid key/value\n");
        }
    } else if (strcmp(cmd, "upgrade") == 0) {
        upgrade_status();
    } else if (strcmp(cmd, "upgrade apply") == 0) {
        static uint8_t pk[8192];
        char next[16];
        upgrade_next_version(next, sizeof(next));
        int n = upgrade_build_demo(pk, sizeof(pk), next);
        if (n > 0)
            upgrade_apply(pk, (uint32_t)n);
        else
            terminal_print("upgrade: cannot build package\n");
    } else if (strcmp(cmd, "upgrade rollback") == 0) {
        upgrade_rollback();
    } else if (strncmp(cmd, "echo ", 5) == 0) {
        terminal_print(cmd + 5);
        terminal_print("\n");
    } else if (strlen(cmd) > 0) {
        terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        terminal_print("Unknown command: ");
        terminal_print(cmd);
        terminal_print("\n");
        terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        terminal_print("Type 'help' for available commands.\n");
    }
}

void kernel_main(struct multiboot_info *mbi)
{
    /* Step 1: Initialize VGA */
    terminal_init();
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    terminal_print("Initializing Basic OS 64-bit...\n");

    /* Step 2: Initialize GDT */
    terminal_print("  GDT... ");
    gdt_init();
    terminal_print("OK\n");

    /* Step 3: Initialize IDT */
    terminal_print("  IDT... ");
    idt_init();
    terminal_print("OK\n");

    /* Step 4: Install ISR handlers */
    terminal_print("  ISR... ");
    isr_install();
    terminal_print("OK\n");

    /* Step 5: Initialize and install IRQ */
    terminal_print("  IRQ... ");
    irq_init();
    irq_install();
    terminal_print("OK\n");

    /* Step 6: Enable interrupts */
    terminal_print("  STI... ");
    sti();
    terminal_print("OK\n");

    /* Step 7: Initialize timer */
    terminal_print("  Timer... ");
    timer_init();
    terminal_print("OK\n");

    /* Step 8: Process multiboot2 info */
    process_multiboot2(mbi);

    /* Enable timer/keyboard IRQs before the boot animation,
     * which blocks on PIT ticks via hlt. */
    outb(0x21, 0xFC);  /* Master: unmask IRQ0 (timer) and IRQ1 (keyboard) */
    outb(0xA1, 0xFF);  /* Slave: mask all */

    /* Step 9: Show boot animation */
    delay(1000000);
    bootscreen_show();

    /* Step 10: Initialize keyboard */
    terminal_print("  Keyboard... ");
    keyboard_init();
    terminal_print("OK\n\n");

    /* Unmask IRQ1 (keyboard) and IRQ0 (timer) */
    terminal_print("  Unmasking IRQ0, IRQ1... ");
    outb(0x21, 0xFC);  /* Master: unmask IRQ0 (timer) and IRQ1 (keyboard) */
    outb(0xA1, 0xFF);  /* Slave: mask all */
    terminal_print("OK\n\n");

    /* Step 11: Initialize kernel heap */
    terminal_print("  Heap... ");
    heap_init();
    terminal_print("OK\n");

    /* Step 12: Initialize the file system (ATA disk or RAM disk) */
    terminal_print("  Filesystem... ");
    fs_init();
    terminal_print(fs_is_persistent() ? "OK (ATA hard disk)\n" : "OK (RAM disk)\n");

    /* Step 13: Load system version and settings */
    terminal_print("  System version... ");
    upgrade_init();
    terminal_print("OK\n");

    terminal_print("  Settings... ");
    settings_init();
    terminal_print("OK\n");

    /* Step 14: Install default .BAS system package (.BAK apps) */
    terminal_print("  Installing system package... ");
    pkg_ensure_defaults();
    terminal_print("OK\n\n");

    /* Main loop */
    terminal_print("Kernel is ready. Type 'help' for commands.\n\n");

    char cmd_buffer[256];
    int cmd_pos = 0;

    print_prompt();

    for (;;) {
        if (keyboard_haschar()) {
            char c = keyboard_getchar();

            if (c == '\n') {
                terminal_putchar('\n');
                cmd_buffer[cmd_pos] = '\0';
                handle_command(cmd_buffer);
                cmd_pos = 0;
                print_prompt();
            } else if (c == '\b') {
                if (cmd_pos > 0) {
                    cmd_pos--;
                    terminal_putchar('\b');
                }
            } else if (cmd_pos < 255) {
                cmd_buffer[cmd_pos++] = c;
                terminal_putchar(c);
            }
        } else {
            /* Idle: wait for interrupt */
            __asm__ volatile ("hlt");
        }
    }
}