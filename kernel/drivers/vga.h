#ifndef VGA_H
#define VGA_H

#include "../types.h"

// VGA text mode constants
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// VGA graphics mode constants (mode 13h)
#define VGA_GRAPHICS_WIDTH 320
#define VGA_GRAPHICS_HEIGHT 200
#define VGA_GRAPHICS_MEMORY 0xA0000

// VGA colors
typedef enum {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
} vga_color_t;

// Function prototypes
void vga_init(void);
void vga_clear(void);
void vga_putchar(char c);
void vga_write_string(const char* str);
void vga_set_color(vga_color_t fg, vga_color_t bg);
void vga_set_cursor(size_t x, size_t y);

// Graphics mode functions
void vga_set_graphics_mode(void);
void vga_set_text_mode(void);
void vga_put_pixel(int x, int y, uint8_t color);
uint8_t vga_get_pixel(int x, int y);

#endif // VGA_H