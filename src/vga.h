#ifndef VGA_H
#define VGA_H

#include "types.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t *)0xFFFFFFFF800B8000ULL)

/* VGA colors */
enum vga_color {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW        = 14,
    VGA_COLOR_WHITE         = 15,
};

void terminal_init(void);
void terminal_clear(void);
void terminal_putchar(char c);
void terminal_print(const char *str);
void terminal_set_color(uint8_t fg, uint8_t bg);
void terminal_print_hex(uint64_t value);
void terminal_print_dec(int64_t value);
void terminal_set_cursor(int x, int y);
void terminal_get_cursor(int *x, int *y);
void terminal_scroll(void);
void terminal_putchar_at(char c, int x, int y, uint8_t color);

#endif