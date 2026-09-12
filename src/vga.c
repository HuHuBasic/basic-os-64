#include "vga.h"
#include "ports.h"
#include "string.h"

#ifdef SERIAL_DEBUG
/* COM1 串口镜像，仅用于无头测试构建（-DSERIAL_DEBUG） */
static void serial_init(void)
{
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
}

static void serial_putchar(char c)
{
    if (c == '\n') serial_putchar('\r');
    while ((inb(0x3F8 + 5) & 0x20) == 0) { }
    outb(0x3F8, (uint8_t)c);
}
#endif

static int    cursor_x = 0;
static int    cursor_y = 0;
static uint8_t current_color = 0;

static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg)
{
    return (bg << 4) | (fg & 0x0F);
}

static inline uint16_t vga_entry(unsigned char c, uint8_t color)
{
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void vga_update_cursor(void)
{
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_init(void)
{
    current_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
#ifdef SERIAL_DEBUG
    serial_init();
#endif
    terminal_clear();
}

void terminal_clear(void)
{
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', current_color);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
    vga_update_cursor();
}

void terminal_set_color(uint8_t fg, uint8_t bg)
{
    current_color = vga_entry_color(fg, bg);
}

void terminal_scroll(void)
{
    /* Move all lines up by one */
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] = VGA_MEMORY[(y + 1) * VGA_WIDTH + x];
        }
    }
    /* Clear the last line */
    for (int x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', current_color);
    }
}

void terminal_putchar(char c)
{
#ifdef SERIAL_DEBUG
    serial_putchar(c);
#endif
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            VGA_MEMORY[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(' ', current_color);
        }
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7;
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    } else {
        VGA_MEMORY[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(c, current_color);
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VGA_HEIGHT) {
        terminal_scroll();
        cursor_y = VGA_HEIGHT - 1;
    }

    vga_update_cursor();
}

void terminal_print(const char *str)
{
    for (uint64_t i = 0; str[i] != '\0'; i++) {
        terminal_putchar(str[i]);
    }
}

void terminal_print_hex(uint64_t value)
{
    terminal_print("0x");
    if (value == 0) {
        terminal_putchar('0');
        return;
    }

    char buf[32];
    int i = 0;
    while (value > 0) {
        int nibble = value & 0xF;
        buf[i++] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        value >>= 4;
    }
    while (i > 0) {
        terminal_putchar(buf[--i]);
    }
}

void terminal_print_dec(int64_t value)
{
    char buf[32];
    itoa(value, buf, 10);
    terminal_print(buf);
}

void terminal_set_cursor(int x, int y)
{
    if (x >= 0 && x < VGA_WIDTH)
        cursor_x = x;
    if (y >= 0 && y < VGA_HEIGHT)
        cursor_y = y;
    vga_update_cursor();
}

void terminal_get_cursor(int *x, int *y)
{
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}

void terminal_putchar_at(char c, int x, int y, uint8_t color)
{
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(c, color);
    }
}