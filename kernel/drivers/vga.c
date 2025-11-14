#include "vga.h"
#include "../net/eth.h" // for outb

static volatile uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;
static volatile uint8_t* vga_graphics_buffer = (uint8_t*)VGA_GRAPHICS_MEMORY;
static size_t vga_row = 0;
static size_t vga_column = 0;
static uint8_t vga_color = 0;
static int vga_mode = 0; // 0 = text, 1 = graphics

static inline uint8_t vga_entry_color(vga_color_t fg, vga_color_t bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t)uc | (uint16_t)color << 8;
}

void vga_init(void) {
    vga_row = 0;
    vga_column = 0;
    vga_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
}

void vga_clear(void) {
    if (vga_mode == 1) {
        // Graphics mode: clear to black
        for (int i = 0; i < VGA_GRAPHICS_WIDTH * VGA_GRAPHICS_HEIGHT; i++) {
            vga_graphics_buffer[i] = 0;
        }
    } else {
        // Text mode
        for (size_t y = 0; y < VGA_HEIGHT; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                const size_t index = y * VGA_WIDTH + x;
                vga_buffer[index] = vga_entry(' ', vga_color);
            }
        }
        vga_row = 0;
        vga_column = 0;
    }
}

void vga_set_color(vga_color_t fg, vga_color_t bg) {
    vga_color = vga_entry_color(fg, bg);
}

void vga_set_cursor(size_t x, size_t y) {
    if (x < VGA_WIDTH && y < VGA_HEIGHT) {
        vga_column = x;
        vga_row = y;
    }
}

static void vga_scroll(void) {
    // Move all lines up by one
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t src_index = (y + 1) * VGA_WIDTH + x;
            const size_t dst_index = y * VGA_WIDTH + x;
            vga_buffer[dst_index] = vga_buffer[src_index];
        }
    }
    
    // Clear the last line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        vga_buffer[index] = vga_entry(' ', vga_color);
    }
    
    vga_row = VGA_HEIGHT - 1;
    vga_column = 0;
}

void vga_putchar(char c) {
    if (vga_mode == 1) return; // No text output in graphics mode
    if (c == '\n') {
        vga_column = 0;
        if (++vga_row == VGA_HEIGHT) {
            vga_scroll();
        }
        return;
    }
    
    if (c == '\r') {
        vga_column = 0;
        return;
    }
    
    if (c == '\b') {
        // Backspace handling
        if (vga_column > 0) {
            vga_column--;
            const size_t index = vga_row * VGA_WIDTH + vga_column;
            vga_buffer[index] = vga_entry(' ', vga_color);
        } else if (vga_row > 0) {
            // Move to previous line if at beginning of line
            vga_row--;
            vga_column = VGA_WIDTH - 1;
            // Find the last non-space character on the previous line
            while (vga_column > 0) {
                const size_t index = vga_row * VGA_WIDTH + vga_column;
                if ((vga_buffer[index] & 0xFF) != ' ') {
                    vga_column++;
                    break;
                }
                vga_column--;
            }
        }
        return;
    }
    
    if (c == '\t') {
        vga_column = (vga_column + 8) & ~(8 - 1);
        if (vga_column >= VGA_WIDTH) {
            vga_column = 0;
            if (++vga_row == VGA_HEIGHT) {
                vga_scroll();
            }
        }
        return;
    }
    
    const size_t index = vga_row * VGA_WIDTH + vga_column;
    vga_buffer[index] = vga_entry(c, vga_color);
    
    if (++vga_column == VGA_WIDTH) {
        vga_column = 0;
        if (++vga_row == VGA_HEIGHT) {
            vga_scroll();
        }
    }
}

void vga_write_string(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

// Graphics mode functions
void vga_set_graphics_mode(void) {
    // Set VGA mode 13h (320x200x256)
    __asm__ volatile (
        "mov $0x13, %%ax\n"
        "int $0x10\n"
        ::: "ax"
    );
    vga_mode = 1;
}

void vga_set_text_mode(void) {
    // Set VGA mode 3 (80x25 text)
    __asm__ volatile (
        "mov $0x03, %%ax\n"
        "int $0x10\n"
        ::: "ax"
    );
    vga_mode = 0;
}

void vga_put_pixel(int x, int y, uint8_t color) {
    if (vga_mode != 1) return;
    if (x < 0 || x >= VGA_GRAPHICS_WIDTH || y < 0 || y >= VGA_GRAPHICS_HEIGHT) return;
    vga_graphics_buffer[y * VGA_GRAPHICS_WIDTH + x] = color;
}

uint8_t vga_get_pixel(int x, int y) {
    if (vga_mode != 1) return 0;
    if (x < 0 || x >= VGA_GRAPHICS_WIDTH || y < 0 || y >= VGA_GRAPHICS_HEIGHT) return 0;
    return vga_graphics_buffer[y * VGA_GRAPHICS_WIDTH + x];
}