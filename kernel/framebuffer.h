#ifndef MYOS_FRAMEBUFFER_H
#define MYOS_FRAMEBUFFER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    volatile uint32_t *pixels;
    uint32_t width;
    uint32_t height;
    uint32_t pitch_pixels;
    uint8_t red_position;
    uint8_t green_position;
    uint8_t blue_position;
} framebuffer_t;

bool framebuffer_initialize(uint32_t boot_info_address, framebuffer_t *framebuffer);
void framebuffer_clear(const framebuffer_t *framebuffer, uint32_t color);
void framebuffer_fill_rect(const framebuffer_t *framebuffer, uint32_t x, uint32_t y,
                           uint32_t width, uint32_t height, uint32_t color);

#endif
