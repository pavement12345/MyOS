#include "e1000.h"

#include "virtual.h"

#define E1000_VENDOR_ID 0x8086U
#define E1000_DEVICE_ID 0x100EU
#define E1000_MMIO_WINDOW_SIZE 0x6000U
#define E1000_BAR0_OFFSET 0x10U
#define E1000_PCI_COMMAND_OFFSET 0x04U
#define PCI_COMMAND_MEMORY_SPACE 0x0002U
#define E1000_CTRL 0x0000U
#define E1000_STATUS 0x0008U
#define E1000_RAL 0x5400U
#define E1000_RAH 0x5404U
#define E1000_CTRL_RESET (1U << 26)
#define E1000_STATUS_LINK_UP (1U << 1)
#define E1000_RESET_POLL_LIMIT 1000000U

static volatile uint32_t *registers;
static uint8_t mac_address[6];
static bool initialized;
static bool link_up;

static uint32_t register_read(uint32_t offset) {
    return registers[offset / sizeof(uint32_t)];
}

static void register_write(uint32_t offset, uint32_t value) {
    registers[offset / sizeof(uint32_t)] = value;
}

static bool read_mac_address(void) {
    uint32_t low = register_read(E1000_RAL);
    uint32_t high = register_read(E1000_RAH);
    mac_address[0] = (uint8_t)low;
    mac_address[1] = (uint8_t)(low >> 8);
    mac_address[2] = (uint8_t)(low >> 16);
    mac_address[3] = (uint8_t)(low >> 24);
    mac_address[4] = (uint8_t)high;
    mac_address[5] = (uint8_t)(high >> 8);

    bool all_zero = true;
    bool all_ones = true;
    for (uint32_t i = 0U; i < sizeof(mac_address); ++i) {
        all_zero = all_zero && mac_address[i] == 0U;
        all_ones = all_ones && mac_address[i] == 0xFFU;
    }
    return !all_zero && !all_ones;
}

bool e1000_initialize(const pci_device_t *device) {
    if (initialized || device == 0 || device->vendor_id != E1000_VENDOR_ID ||
        device->device_id != E1000_DEVICE_ID ||
        device->class_code != 0x02U || device->subclass != 0x00U) {
        return false;
    }

    uint32_t bar0;
    uint16_t command;
    if (!pci_config_read32(device, E1000_BAR0_OFFSET, &bar0) ||
        !pci_config_read16(device, E1000_PCI_COMMAND_OFFSET, &command) ||
        (bar0 & 1U) != 0U) {
        return false;
    }

    uint32_t bar_type = (bar0 >> 1) & 3U;
    if (bar_type != 0U && bar_type != 2U) {
        return false;
    }
    uint64_t mmio_base = (uint64_t)(bar0 & ~0x0FU);
    if (bar_type == 2U) {
        uint32_t bar1;
        if (!pci_config_read32(device, E1000_BAR0_OFFSET + 4U, &bar1) ||
            bar1 != 0U) {
            return false;
        }
    }
    if (mmio_base == 0U ||
        mmio_base > UINT64_C(0x100000000) - E1000_MMIO_WINDOW_SIZE) {
        return false;
    }

    for (uint32_t offset = 0U; offset < E1000_MMIO_WINDOW_SIZE;
         offset += MYOS_VIRTUAL_PAGE_SIZE) {
        if (!virtual_memory_mark_mmio(mmio_base + offset)) {
            return false;
        }
    }

    if (!pci_config_write16(device, E1000_PCI_COMMAND_OFFSET,
                            (uint16_t)(command | PCI_COMMAND_MEMORY_SPACE))) {
        return false;
    }

    registers = (volatile uint32_t *)(uintptr_t)mmio_base;
    if (register_read(E1000_STATUS) == UINT32_MAX) {
        registers = 0;
        return false;
    }

    register_write(E1000_CTRL, register_read(E1000_CTRL) | E1000_CTRL_RESET);
    uint32_t poll = 0U;
    while ((register_read(E1000_CTRL) & E1000_CTRL_RESET) != 0U &&
           poll < E1000_RESET_POLL_LIMIT) {
        ++poll;
    }
    if (poll == E1000_RESET_POLL_LIMIT ||
        register_read(E1000_STATUS) == UINT32_MAX ||
        !read_mac_address()) {
        registers = 0;
        return false;
    }

    link_up = (register_read(E1000_STATUS) & E1000_STATUS_LINK_UP) != 0U;
    initialized = true;
    return true;
}

bool e1000_is_initialized(void) {
    return initialized;
}

bool e1000_link_is_up(void) {
    return initialized && link_up;
}

const uint8_t *e1000_mac_address(void) {
    return initialized ? mac_address : 0;
}
