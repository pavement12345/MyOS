#include "framebuffer.h"

#define MULTIBOOT2_INFO_LIMIT (1024U * 1024U)
#define IDENTITY_MAP_LIMIT UINT64_C(0x100000000)

static uint32_t read_u32(const uint8_t *bytes) {
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static uint64_t read_u64(const uint8_t *bytes) {
    return (uint64_t)read_u32(bytes) | ((uint64_t)read_u32(bytes + 4) << 32);
}

static bool parse_framebuffer_tag(const uint8_t *tag, uint32_t size,
                                  framebuffer_t *framebuffer) {
    if (size < 38U || tag[29] != 1U || tag[28] != 32U) {
        return false;
    }

    uint64_t address = read_u64(tag + 8);
    uint32_t pitch = read_u32(tag + 16);
    uint32_t width = read_u32(tag + 20);
    uint32_t height = read_u32(tag + 24);
    uint8_t red_position = tag[32];
    uint8_t red_size = tag[33];
    uint8_t green_position = tag[34];
    uint8_t green_size = tag[35];
    uint8_t blue_position = tag[36];
    uint8_t blue_size = tag[37];

    if (width < 640U || width > 1920U || height < 480U || height > 1080U ||
        red_size != 8U || green_size != 8U || blue_size != 8U ||
        red_position > 24U || green_position > 24U || blue_position > 24U ||
        (red_position == green_position) || (red_position == blue_position) ||
        (green_position == blue_position) || pitch < width * 4U || (pitch & 3U) != 0U ||
        (address & 3U) != 0U) {
        return false;
    }

    uint64_t framebuffer_size = (uint64_t)pitch * height;
    if (address >= IDENTITY_MAP_LIMIT ||
        framebuffer_size > IDENTITY_MAP_LIMIT - address) {
        return false;
    }

    framebuffer->pixels = (volatile uint32_t *)(uintptr_t)address;
    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->pitch_pixels = pitch / 4U;
    framebuffer->red_position = red_position;
    framebuffer->green_position = green_position;
    framebuffer->blue_position = blue_position;
    return true;
}

bool framebuffer_initialize(uint32_t boot_info_address, framebuffer_t *framebuffer) {
    if (framebuffer == 0 || boot_info_address == 0U ||
        (boot_info_address & 7U) != 0U ||
        boot_info_address > UINT32_MAX - 8U) {
        return false;
    }

    const uint8_t *info = (const uint8_t *)(uintptr_t)boot_info_address;
    uint32_t total_size = read_u32(info);
    if (total_size < 16U || total_size > MULTIBOOT2_INFO_LIMIT ||
        (uint64_t)boot_info_address + total_size > IDENTITY_MAP_LIMIT) {
        return false;
    }

    uint32_t offset = 8U;
    while (offset <= total_size - 8U) {
        const uint8_t *tag = info + offset;
        uint32_t type = read_u32(tag);
        uint32_t size = read_u32(tag + 4);
        if (size < 8U || size > total_size - offset) {
            return false;
        }
        if (type == 0U) {
            return size == 8U ? false : false;
        }
        if (type == 8U) {
            return parse_framebuffer_tag(tag, size, framebuffer);
        }

        uint32_t aligned_size = (size + 7U) & ~7U;
        if (aligned_size > total_size - offset) {
            return false;
        }
        offset += aligned_size;
    }
    return false;
}

static uint32_t encode_color(const framebuffer_t *framebuffer, uint32_t color) {
    uint32_t red = (color >> 16) & 0xFFU;
    uint32_t green = (color >> 8) & 0xFFU;
    uint32_t blue = color & 0xFFU;
    return (red << framebuffer->red_position) |
           (green << framebuffer->green_position) |
           (blue << framebuffer->blue_position);
}

void framebuffer_clear(const framebuffer_t *framebuffer, uint32_t color) {
    framebuffer_fill_rect(framebuffer, 0U, 0U, framebuffer->width,
                          framebuffer->height, color);
}

void framebuffer_fill_rect(const framebuffer_t *framebuffer, uint32_t x, uint32_t y,
                           uint32_t width, uint32_t height, uint32_t color) {
    if (framebuffer == 0 || framebuffer->pixels == 0 ||
        x >= framebuffer->width || y >= framebuffer->height) {
        return;
    }

    if (width > framebuffer->width - x) {
        width = framebuffer->width - x;
    }
    if (height > framebuffer->height - y) {
        height = framebuffer->height - y;
    }

    uint32_t pixel = encode_color(framebuffer, color);
    for (uint32_t row = 0U; row < height; ++row) {
        volatile uint32_t *line = framebuffer->pixels +
                                  (uint64_t)(y + row) * framebuffer->pitch_pixels + x;
        for (uint32_t column = 0U; column < width; ++column) {
            line[column] = pixel;
        }
    }
}
