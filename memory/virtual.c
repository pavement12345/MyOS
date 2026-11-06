#include "virtual.h"
#include "physical.h"

#define PAGE_PRESENT UINT64_C(0x001)
#define PAGE_WRITABLE UINT64_C(0x002)
#define PAGE_HUGE UINT64_C(0x080)
#define PAGE_ADDRESS_MASK UINT64_C(0x000FFFFFFFFFF000)
#define LOWER_CANONICAL_LIMIT UINT64_C(0x0000800000000000)
#define PAGE_TABLE_ENTRIES 512U

extern uint64_t pml4_table[PAGE_TABLE_ENTRIES];

static bool aligned_addresses(uint64_t virtual_address, uint64_t physical_address) {
    return (virtual_address & (MYOS_VIRTUAL_PAGE_SIZE - 1U)) == 0U &&
           (physical_address & (MYOS_VIRTUAL_PAGE_SIZE - 1U)) == 0U &&
           virtual_address < LOWER_CANONICAL_LIMIT &&
           physical_address < UINT64_C(0x100000000);
}

static bool allocate_table(uint64_t **table) {
    uint64_t physical_address;
    if (!physical_frame_allocate(&physical_address)) {
        return false;
    }

    *table = (uint64_t *)(uintptr_t)physical_address;
    for (uint32_t i = 0U; i < PAGE_TABLE_ENTRIES; ++i) {
        (*table)[i] = 0U;
    }
    return true;
}

static bool table_is_empty(const uint64_t *table) {
    for (uint32_t i = 0U; i < PAGE_TABLE_ENTRIES; ++i) {
        if ((table[i] & PAGE_PRESENT) != 0U) {
            return false;
        }
    }
    return true;
}

bool virtual_memory_map_page(uint64_t virtual_address, uint64_t physical_address) {
    if (!aligned_addresses(virtual_address, physical_address)) {
        return false;
    }

    uint32_t pml4_index = (uint32_t)((virtual_address >> 39) & 0x1FFU);
    uint32_t pdpt_index = (uint32_t)((virtual_address >> 30) & 0x1FFU);
    uint32_t pd_index = (uint32_t)((virtual_address >> 21) & 0x1FFU);
    uint32_t pt_index = (uint32_t)((virtual_address >> 12) & 0x1FFU);
    uint64_t *pdpt;
    uint64_t *page_directory;
    uint64_t *page_table;
    bool new_pdpt = false;
    bool new_page_directory = false;
    bool new_page_table = false;
    uint64_t pml4_entry = pml4_table[pml4_index];

    if ((pml4_entry & PAGE_PRESENT) != 0U) {
        pdpt = (uint64_t *)(uintptr_t)(pml4_entry & PAGE_ADDRESS_MASK);
    } else if (allocate_table(&pdpt)) {
        new_pdpt = true;
    } else {
        return false;
    }

    uint64_t pdpt_entry = pdpt[pdpt_index];
    if ((pdpt_entry & PAGE_PRESENT) != 0U) {
        if ((pdpt_entry & PAGE_HUGE) != 0U) {
            if (new_pdpt) {
                (void)physical_frame_release((uint64_t)(uintptr_t)pdpt);
            }
            return false;
        }
        page_directory = (uint64_t *)(uintptr_t)(pdpt_entry & PAGE_ADDRESS_MASK);
    } else if (allocate_table(&page_directory)) {
        new_page_directory = true;
    } else {
        if (new_pdpt) {
            (void)physical_frame_release((uint64_t)(uintptr_t)pdpt);
        }
        return false;
    }

    uint64_t directory_entry = page_directory[pd_index];
    if ((directory_entry & PAGE_PRESENT) != 0U) {
        if ((directory_entry & PAGE_HUGE) != 0U) {
            if (new_page_directory) {
                (void)physical_frame_release((uint64_t)(uintptr_t)page_directory);
            }
            if (new_pdpt) {
                (void)physical_frame_release((uint64_t)(uintptr_t)pdpt);
            }
            return false;
        }
        page_table = (uint64_t *)(uintptr_t)(directory_entry & PAGE_ADDRESS_MASK);
    } else if (allocate_table(&page_table)) {
        new_page_table = true;
    } else {
        if (new_page_directory) {
            (void)physical_frame_release((uint64_t)(uintptr_t)page_directory);
        }
        if (new_pdpt) {
            (void)physical_frame_release((uint64_t)(uintptr_t)pdpt);
        }
        return false;
    }

    if ((page_table[pt_index] & PAGE_PRESENT) != 0U) {
        if (new_page_table) {
            (void)physical_frame_release((uint64_t)(uintptr_t)page_table);
        }
        if (new_page_directory) {
            (void)physical_frame_release((uint64_t)(uintptr_t)page_directory);
        }
        if (new_pdpt) {
            (void)physical_frame_release((uint64_t)(uintptr_t)pdpt);
        }
        return false;
    }

    if (new_pdpt) {
        pml4_table[pml4_index] = (uint64_t)(uintptr_t)pdpt |
                                 PAGE_PRESENT | PAGE_WRITABLE;
    } else {
        pml4_table[pml4_index] |= PAGE_WRITABLE;
    }
    if (new_page_directory) {
        pdpt[pdpt_index] = (uint64_t)(uintptr_t)page_directory |
                           PAGE_PRESENT | PAGE_WRITABLE;
    } else {
        pdpt[pdpt_index] |= PAGE_WRITABLE;
    }
    if (new_page_table) {
        page_directory[pd_index] = (uint64_t)(uintptr_t)page_table |
                                   PAGE_PRESENT | PAGE_WRITABLE;
    } else {
        page_directory[pd_index] |= PAGE_WRITABLE;
    }

    page_table[pt_index] = physical_address | PAGE_PRESENT | PAGE_WRITABLE;
    __asm__ volatile ("invlpg (%0)" : : "r"(virtual_address) : "memory");
    return true;
}

bool virtual_memory_unmap_page(uint64_t virtual_address, uint64_t *physical_address) {
    if (physical_address == 0 ||
        (virtual_address & (MYOS_VIRTUAL_PAGE_SIZE - 1U)) != 0U ||
        virtual_address >= LOWER_CANONICAL_LIMIT) {
        return false;
    }

    uint32_t pml4_index = (uint32_t)((virtual_address >> 39) & 0x1FFU);
    uint32_t pdpt_index = (uint32_t)((virtual_address >> 30) & 0x1FFU);
    uint32_t pd_index = (uint32_t)((virtual_address >> 21) & 0x1FFU);
    uint32_t pt_index = (uint32_t)((virtual_address >> 12) & 0x1FFU);
    uint64_t *pdpt;
    uint64_t *page_directory;
    uint64_t *page_table;

    if ((pml4_table[pml4_index] & PAGE_PRESENT) == 0U ||
        (pml4_table[pml4_index] & PAGE_HUGE) != 0U) {
        return false;
    }
    pdpt = (uint64_t *)(uintptr_t)(pml4_table[pml4_index] & PAGE_ADDRESS_MASK);

    if ((pdpt[pdpt_index] & PAGE_PRESENT) == 0U ||
        (pdpt[pdpt_index] & PAGE_HUGE) != 0U) {
        return false;
    }
    page_directory = (uint64_t *)(uintptr_t)(pdpt[pdpt_index] & PAGE_ADDRESS_MASK);

    if ((page_directory[pd_index] & PAGE_PRESENT) == 0U ||
        (page_directory[pd_index] & PAGE_HUGE) != 0U) {
        return false;
    }
    page_table = (uint64_t *)(uintptr_t)(page_directory[pd_index] & PAGE_ADDRESS_MASK);

    uint64_t page_entry = page_table[pt_index];
    if ((page_entry & PAGE_PRESENT) == 0U) {
        return false;
    }

    *physical_address = page_entry & PAGE_ADDRESS_MASK;
    page_table[pt_index] = 0U;
    __asm__ volatile ("invlpg (%0)" : : "r"(virtual_address) : "memory");

    if (table_is_empty(page_table)) {
        uint64_t table_address = page_directory[pd_index] & PAGE_ADDRESS_MASK;
        page_directory[pd_index] = 0U;
        (void)physical_frame_release(table_address);

        if (table_is_empty(page_directory)) {
            uint64_t directory_address = pdpt[pdpt_index] & PAGE_ADDRESS_MASK;
            pdpt[pdpt_index] = 0U;
            (void)physical_frame_release(directory_address);

            if (table_is_empty(pdpt)) {
                uint64_t pdpt_address = pml4_table[pml4_index] & PAGE_ADDRESS_MASK;
                pml4_table[pml4_index] = 0U;
                (void)physical_frame_release(pdpt_address);
            }
        }
    }
    return true;
}
