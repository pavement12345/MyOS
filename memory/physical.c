#include "physical.h"

#define MULTIBOOT2_INFO_LIMIT (1024U * 1024U)
#define IDENTITY_MAP_LIMIT UINT64_C(0x100000000)
#define FRAME_COUNT ((uint32_t)(IDENTITY_MAP_LIMIT / MYOS_PHYSICAL_FRAME_SIZE))
#define FRAME_BITMAP_SIZE (FRAME_COUNT / 8U)
#define LOW_MEMORY_LIMIT UINT64_C(0x100000)

extern uint8_t __kernel_start;
extern uint8_t __kernel_end;

static uint8_t frame_bitmap[FRAME_BITMAP_SIZE];
static uint8_t allocated_bitmap[FRAME_BITMAP_SIZE];
static uint32_t free_frame_count;
static uint32_t next_frame_hint;

static uint32_t read_u32(const uint8_t *bytes) {
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static uint64_t read_u64(const uint8_t *bytes) {
    return (uint64_t)read_u32(bytes) | ((uint64_t)read_u32(bytes + 4) << 32);
}

static bool bit_is_set(const uint8_t *bitmap, uint32_t frame) {
    return (bitmap[frame / 8U] & (uint8_t)(1U << (frame % 8U))) != 0U;
}

static void set_bit(uint8_t *bitmap, uint32_t frame) {
    bitmap[frame / 8U] |= (uint8_t)(1U << (frame % 8U));
}

static void clear_bit(uint8_t *bitmap, uint32_t frame) {
    bitmap[frame / 8U] &= (uint8_t)~(1U << (frame % 8U));
}

static void set_range(uint64_t start, uint64_t end) {
    if (start >= IDENTITY_MAP_LIMIT) {
        return;
    }
    if (end > IDENTITY_MAP_LIMIT) {
        end = IDENTITY_MAP_LIMIT;
    }
    uint32_t first = (uint32_t)(start / MYOS_PHYSICAL_FRAME_SIZE);
    uint32_t last = (uint32_t)((end + MYOS_PHYSICAL_FRAME_SIZE - 1U) /
                               MYOS_PHYSICAL_FRAME_SIZE);
    if (last > FRAME_COUNT) {
        last = FRAME_COUNT;
    }
    for (uint32_t frame = first; frame < last; ++frame) {
        if (!bit_is_set(frame_bitmap, frame)) {
            set_bit(frame_bitmap, frame);
            --free_frame_count;
        }
    }
}

static void clear_available_range(uint64_t start, uint64_t end) {
    if (start >= IDENTITY_MAP_LIMIT) {
        return;
    }
    if (end > IDENTITY_MAP_LIMIT) {
        end = IDENTITY_MAP_LIMIT;
    }
    start = (start + MYOS_PHYSICAL_FRAME_SIZE - 1U) &
            ~((uint64_t)MYOS_PHYSICAL_FRAME_SIZE - 1U);
    end &= ~((uint64_t)MYOS_PHYSICAL_FRAME_SIZE - 1U);
    if (end <= start) {
        return;
    }
    uint32_t first = (uint32_t)(start / MYOS_PHYSICAL_FRAME_SIZE);
    uint32_t last = (uint32_t)(end / MYOS_PHYSICAL_FRAME_SIZE);
    for (uint32_t frame = first; frame < last; ++frame) {
        if (bit_is_set(frame_bitmap, frame)) {
            clear_bit(frame_bitmap, frame);
            ++free_frame_count;
        }
    }
}

static bool validate_memory_map(const uint8_t *tag, uint32_t tag_size,
                                uint32_t *entry_size) {
    if (tag_size < 40U) {
        return false;
    }

    *entry_size = read_u32(tag + 8U);
    uint32_t entry_version = read_u32(tag + 12U);
    if (*entry_size < 24U || entry_version != 0U ||
        (tag_size - 16U) % *entry_size != 0U) {
        return false;
    }

    uint32_t count = (tag_size - 16U) / *entry_size;
    if (count == 0U) {
        return false;
    }
    for (uint32_t i = 0U; i < count; ++i) {
        const uint8_t *entry = tag + 16U + i * *entry_size;
        uint64_t base = read_u64(entry);
        uint64_t length = read_u64(entry + 8U);
        if (length != 0U && base > UINT64_MAX - length) {
            return false;
        }
    }
    return true;
}

static void apply_memory_map(const uint8_t *tag, uint32_t tag_size,
                             uint32_t entry_size) {
    uint32_t count = (tag_size - 16U) / entry_size;

    for (uint32_t i = 0U; i < count; ++i) {
        const uint8_t *entry = tag + 16U + i * entry_size;
        uint64_t base = read_u64(entry);
        uint64_t length = read_u64(entry + 8U);
        uint32_t type = read_u32(entry + 16U);
        if (type == 1U && length != 0U) {
            clear_available_range(base, base + length);
        }
    }

    for (uint32_t i = 0U; i < count; ++i) {
        const uint8_t *entry = tag + 16U + i * entry_size;
        uint64_t base = read_u64(entry);
        uint64_t length = read_u64(entry + 8U);
        uint32_t type = read_u32(entry + 16U);
        if (type != 1U && length != 0U) {
            set_range(base, base + length);
        }
    }
}

bool physical_memory_initialize(uint32_t boot_info_address,
                                const framebuffer_t *framebuffer,
                                uint32_t *available_frames) {
    if (framebuffer == 0 || framebuffer->pixels == 0 ||
        available_frames == 0 || boot_info_address == 0U ||
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

    const uint8_t *memory_map_tag = 0;
    uint32_t memory_map_size = 0U;
    uint32_t memory_entry_size = 0U;
    bool found_end_tag = false;
    uint32_t offset = 8U;
    while (offset <= total_size - 8U) {
        const uint8_t *tag = info + offset;
        uint32_t type = read_u32(tag);
        uint32_t size = read_u32(tag + 4U);
        if (size < 8U || size > total_size - offset) {
            return false;
        }
        if (type == 0U) {
            if (size != 8U || offset + size != total_size) {
                return false;
            }
            found_end_tag = true;
            break;
        }
        if (type == 6U) {
            if (memory_map_tag != 0 ||
                !validate_memory_map(tag, size, &memory_entry_size)) {
                return false;
            }
            memory_map_tag = tag;
            memory_map_size = size;
        }

        uint32_t aligned_size = (size + 7U) & ~7U;
        if (aligned_size > total_size - offset) {
            return false;
        }
        offset += aligned_size;
    }
    if (!found_end_tag || memory_map_tag == 0) {
        return false;
    }

    for (uint32_t i = 0U; i < FRAME_BITMAP_SIZE; ++i) {
        frame_bitmap[i] = 0xFFU;
        allocated_bitmap[i] = 0U;
    }
    free_frame_count = 0U;
    next_frame_hint = 0U;
    apply_memory_map(memory_map_tag, memory_map_size, memory_entry_size);

    set_range(0U, LOW_MEMORY_LIMIT);
    set_range((uint64_t)(uintptr_t)&__kernel_start,
              (uint64_t)(uintptr_t)&__kernel_end);
    set_range(boot_info_address, (uint64_t)boot_info_address + total_size);
    uint64_t framebuffer_start = (uint64_t)(uintptr_t)framebuffer->pixels;
    uint64_t framebuffer_size = (uint64_t)framebuffer->pitch_pixels *
                               framebuffer->height * sizeof(uint32_t);
    set_range(framebuffer_start, framebuffer_start + framebuffer_size);

    *available_frames = free_frame_count;
    return free_frame_count != 0U;
}

bool physical_frame_allocate(uint64_t *physical_address) {
    if (physical_address == 0 || free_frame_count == 0U) {
        return false;
    }

    uint32_t frame = next_frame_hint;
    for (uint32_t examined = 0U; examined < FRAME_COUNT; ++examined) {
        if (!bit_is_set(frame_bitmap, frame)) {
            uint32_t allocated_frame = frame;
            set_bit(frame_bitmap, frame);
            set_bit(allocated_bitmap, frame);
            --free_frame_count;
            ++frame;
            if (frame == FRAME_COUNT) {
                frame = 0U;
            }
            next_frame_hint = frame;
            *physical_address = (uint64_t)allocated_frame * MYOS_PHYSICAL_FRAME_SIZE;
            return true;
        }
        ++frame;
        if (frame == FRAME_COUNT) {
            frame = 0U;
        }
    }
    return false;
}

bool physical_frame_release(uint64_t physical_address) {
    if ((physical_address & (MYOS_PHYSICAL_FRAME_SIZE - 1U)) != 0U ||
        physical_address >= IDENTITY_MAP_LIMIT) {
        return false;
    }

    uint32_t frame = (uint32_t)(physical_address / MYOS_PHYSICAL_FRAME_SIZE);
    if (!bit_is_set(allocated_bitmap, frame) || !bit_is_set(frame_bitmap, frame)) {
        return false;
    }
    clear_bit(allocated_bitmap, frame);
    clear_bit(frame_bitmap, frame);
    ++free_frame_count;
    next_frame_hint = frame;
    return true;
}
