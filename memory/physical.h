#ifndef MYOS_PHYSICAL_MEMORY_H
#define MYOS_PHYSICAL_MEMORY_H

#include "framebuffer.h"

#define MYOS_PHYSICAL_FRAME_SIZE 4096U

bool physical_memory_initialize(uint32_t boot_info_address,
                                const framebuffer_t *framebuffer,
                                uint32_t *available_frames);
bool physical_frame_allocate(uint64_t *physical_address);
bool physical_frame_release(uint64_t physical_address);

#endif
