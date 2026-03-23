#include "framebuffer.h"

static framebuffer_info_t g_fb_info;

static inline volatile uint8_t* fb_base_u8(void) {
    return (volatile uint8_t*)(uintptr_t)g_fb_info.phys_addr;
}

void fb_init_scaffold(void) {
    g_fb_info.available = false;
    g_fb_info.backend = FB_BACKEND_NONE;
    g_fb_info.phys_addr = 0;
    g_fb_info.width = 0;
    g_fb_info.height = 0;
    g_fb_info.pitch = 0;
    g_fb_info.bpp = 0;
}

void fb_register_linear_framebuffer(uint32_t phys_addr, uint32_t width, uint32_t height, uint32_t pitch, uint32_t bpp) {
    g_fb_info.available = true;
    g_fb_info.backend = FB_BACKEND_LINEAR;
    g_fb_info.phys_addr = phys_addr;
    g_fb_info.width = width;
    g_fb_info.height = height;
    g_fb_info.pitch = pitch;
    g_fb_info.bpp = bpp;
}

void fb_try_enable_mode13_scaffold(void) {
    // Scaffolding only: assumes graphics memory is mapped at 0xA0000.
    // Actual mode-switch must be done in boot code (real mode) or via VBE.
    g_fb_info.available = true;
    g_fb_info.backend = FB_BACKEND_MODE13;
    g_fb_info.phys_addr = 0xA0000;
    g_fb_info.width = 320;
    g_fb_info.height = 200;
    g_fb_info.pitch = 320;
    g_fb_info.bpp = 8;
}

const framebuffer_info_t* fb_get_info(void) {
    return &g_fb_info;
}

int fb_is_available(void) {
    return g_fb_info.available ? 1 : 0;
}

void fb_clear(uint32_t color) {
    if (!g_fb_info.available || g_fb_info.bpp != 8) {
        return;
    }

    volatile uint8_t* base = fb_base_u8();
    uint8_t value = (uint8_t)(color & 0xFF);

    for (uint32_t y = 0; y < g_fb_info.height; y++) {
        uint32_t row_offset = y * g_fb_info.pitch;
        for (uint32_t x = 0; x < g_fb_info.width; x++) {
            base[row_offset + x] = value;
        }
    }
}

void fb_putpixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!g_fb_info.available || g_fb_info.bpp != 8) {
        return;
    }
    if (x >= g_fb_info.width || y >= g_fb_info.height) {
        return;
    }

    volatile uint8_t* base = fb_base_u8();
    base[y * g_fb_info.pitch + x] = (uint8_t)(color & 0xFF);
}

void fb_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    if (!g_fb_info.available || g_fb_info.bpp != 8) {
        return;
    }

    uint32_t x_end = x + width;
    uint32_t y_end = y + height;

    if (x_end > g_fb_info.width) x_end = g_fb_info.width;
    if (y_end > g_fb_info.height) y_end = g_fb_info.height;

    for (uint32_t py = y; py < y_end; py++) {
        for (uint32_t px = x; px < x_end; px++) {
            fb_putpixel(px, py, color);
        }
    }
}

void fb_demo_gradient(void) {
    if (!g_fb_info.available || g_fb_info.bpp != 8) {
        return;
    }

    for (uint32_t y = 0; y < g_fb_info.height; y++) {
        for (uint32_t x = 0; x < g_fb_info.width; x++) {
            uint8_t c = (uint8_t)(((x * 32) / g_fb_info.width) + ((y * 8) / g_fb_info.height));
            fb_putpixel(x, y, c);
        }
    }

    fb_fill_rect(20, 20, 100, 30, 60);
    fb_fill_rect(30, 30, 80, 10, 10);
}
