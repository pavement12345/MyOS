#ifndef MYOS_VIRTUAL_MEMORY_H
#define MYOS_VIRTUAL_MEMORY_H

#include <stdbool.h>
#include <stdint.h>

#define MYOS_VIRTUAL_PAGE_SIZE 4096U

bool virtual_memory_map_page(uint64_t virtual_address, uint64_t physical_address);
bool virtual_memory_unmap_page(uint64_t virtual_address, uint64_t *physical_address);

#endif
