#include "heap.h"
#include "physical.h"
#include "virtual.h"

#define HEAP_BASE UINT64_C(0x1000000000)
#define HEAP_MAX_SIZE (64U * 1024U * 1024U)
#define HEAP_GROW_PAGES 16U
#define HEAP_ALIGNMENT 16U

typedef struct heap_block {
    size_t size;
    struct heap_block *next;
    bool free;
    uint8_t reserved[15];
} heap_block_t;

_Static_assert(sizeof(heap_block_t) == 32U, "heap block header must remain 16-byte aligned");

static heap_block_t *heap_blocks;
static uint64_t heap_mapped_size;

static bool align_size(size_t size, size_t *aligned_size) {
    if (size > SIZE_MAX - (HEAP_ALIGNMENT - 1U)) {
        return false;
    }
    *aligned_size = (size + HEAP_ALIGNMENT - 1U) & ~(size_t)(HEAP_ALIGNMENT - 1U);
    return true;
}

static bool grow_heap(void) {
    uint64_t growth_size = (uint64_t)HEAP_GROW_PAGES * MYOS_VIRTUAL_PAGE_SIZE;
    if (heap_mapped_size > HEAP_MAX_SIZE ||
        growth_size > HEAP_MAX_SIZE - heap_mapped_size) {
        return false;
    }

    uint32_t mapped_pages = 0U;
    for (; mapped_pages < HEAP_GROW_PAGES; ++mapped_pages) {
        uint64_t physical_address;
        uint64_t virtual_address = HEAP_BASE + heap_mapped_size +
                                  (uint64_t)mapped_pages * MYOS_VIRTUAL_PAGE_SIZE;
        if (!physical_frame_allocate(&physical_address)) {
            break;
        }
        if (!virtual_memory_map_page(virtual_address, physical_address)) {
            (void)physical_frame_release(physical_address);
            break;
        }
    }

    if (mapped_pages != HEAP_GROW_PAGES) {
        while (mapped_pages != 0U) {
            --mapped_pages;
            uint64_t virtual_address = HEAP_BASE + heap_mapped_size +
                                       (uint64_t)mapped_pages * MYOS_VIRTUAL_PAGE_SIZE;
            uint64_t physical_address;
            if (virtual_memory_unmap_page(virtual_address, &physical_address)) {
                (void)physical_frame_release(physical_address);
            }
        }
        return false;
    }

    heap_block_t *new_block = (heap_block_t *)(uintptr_t)
                              (HEAP_BASE + heap_mapped_size);
    new_block->size = (size_t)growth_size - sizeof(heap_block_t);
    new_block->next = 0;
    new_block->free = true;
    for (uint32_t i = 0U; i < sizeof(new_block->reserved); ++i) {
        new_block->reserved[i] = 0U;
    }

    if (heap_blocks == 0) {
        heap_blocks = new_block;
    } else {
        heap_block_t *last = heap_blocks;
        while (last->next != 0) {
            last = last->next;
        }
        uint8_t *last_end = (uint8_t *)(last + 1) + last->size;
        if (last->free && last_end == (uint8_t *)new_block) {
            last->size += sizeof(heap_block_t) + new_block->size;
        } else {
            last->next = new_block;
        }
    }
    heap_mapped_size += growth_size;
    return true;
}

void *kernel_heap_allocate(size_t size) {
    size_t aligned_size;
    if (size == 0U || !align_size(size, &aligned_size)) {
        return 0;
    }

    for (;;) {
        heap_block_t *block = heap_blocks;
        while (block != 0) {
            if (block->free && block->size >= aligned_size) {
                size_t remaining = block->size - aligned_size;
                if (remaining > sizeof(heap_block_t)) {
                    heap_block_t *split = (heap_block_t *)
                                          ((uint8_t *)(block + 1) + aligned_size);
                    split->size = remaining - sizeof(heap_block_t);
                    split->next = block->next;
                    split->free = true;
                    for (uint32_t i = 0U; i < sizeof(split->reserved); ++i) {
                        split->reserved[i] = 0U;
                    }
                    block->size = aligned_size;
                    block->next = split;
                }
                block->free = false;
                return block + 1;
            }
            block = block->next;
        }

        if (!grow_heap()) {
            return 0;
        }
    }
}

bool kernel_heap_release(void *pointer) {
    if (pointer == 0) {
        return false;
    }

    heap_block_t *block = heap_blocks;
    while (block != 0 && (void *)(block + 1) != pointer) {
        block = block->next;
    }
    if (block == 0 || block->free) {
        return false;
    }
    block->free = true;

    for (block = heap_blocks; block != 0 && block->next != 0; ) {
        heap_block_t *next = block->next;
        uint8_t *block_end = (uint8_t *)(block + 1) + block->size;
        if (block->free && next->free && block_end == (uint8_t *)next) {
            block->size += sizeof(heap_block_t) + next->size;
            block->next = next->next;
        } else {
            block = next;
        }
    }
    return true;
}
