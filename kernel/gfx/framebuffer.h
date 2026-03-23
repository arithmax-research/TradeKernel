#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "../types.h"

typedef enum {
    FB_BACKEND_NONE = 0,
    FB_BACKEND_LINEAR = 1,
    FB_BACKEND_MODE13 = 2
} fb_backend_t;

typedef struct {
    bool available;
    fb_backend_t backend;
    uint32_t phys_addr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
} framebuffer_info_t;

void fb_init_scaffold(void);
void fb_register_linear_framebuffer(uint32_t phys_addr, uint32_t width, uint32_t height, uint32_t pitch, uint32_t bpp);
void fb_try_enable_mode13_scaffold(void);

const framebuffer_info_t* fb_get_info(void);
int fb_is_available(void);

void fb_clear(uint32_t color);
void fb_putpixel(uint32_t x, uint32_t y, uint32_t color);
void fb_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
void fb_demo_gradient(void);

#endif // FRAMEBUFFER_H
