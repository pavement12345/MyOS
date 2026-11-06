#include "pci.h"

#define PCI_CONFIG_ADDRESS 0xCF8U
#define PCI_CONFIG_DATA 0xCFCU
#define PCI_CONFIG_ENABLE 0x80000000U
#define PCI_MAX_BUSES 256U
#define PCI_MAX_DEVICES 32U
#define PCI_MAX_FUNCTIONS 8U
#define PCI_VENDOR_ABSENT 0xFFFFU

static pci_device_t devices[PCI_DEVICE_LIMIT];
static uint32_t device_count;

static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t value;
    __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static uint32_t config_address(uint8_t bus, uint8_t device, uint8_t function,
                               uint8_t offset) {
    return PCI_CONFIG_ENABLE |
           ((uint32_t)bus << 16) |
           ((uint32_t)device << 11) |
           ((uint32_t)function << 8) |
           ((uint32_t)offset & 0xFCU);
}

static uint32_t config_read(uint8_t bus, uint8_t device, uint8_t function,
                            uint8_t offset) {
    outl(PCI_CONFIG_ADDRESS,
         config_address(bus, device, function, offset));
    return inl(PCI_CONFIG_DATA);
}

static void record_function(uint8_t bus, uint8_t device, uint8_t function,
                            uint32_t identity, uint32_t class_register,
                            uint32_t header_register, uint32_t interrupt_register) {
    uint8_t header_type = (uint8_t)(header_register >> 16);
    uint32_t index = device_count++;
    devices[index] = (pci_device_t) {
        .bus = bus,
        .device = device,
        .function = function,
        .header_type = header_type,
        .class_code = (uint8_t)(class_register >> 24),
        .subclass = (uint8_t)(class_register >> 16),
        .programming_interface = (uint8_t)(class_register >> 8),
        .interrupt_line = (uint8_t)interrupt_register,
        .vendor_id = (uint16_t)identity,
        .device_id = (uint16_t)(identity >> 16)
    };
}

bool pci_initialize(void) {
    device_count = 0U;

    for (uint32_t bus_index = 0U; bus_index < PCI_MAX_BUSES; ++bus_index) {
        uint8_t bus = (uint8_t)bus_index;
        for (uint32_t device_index = 0U; device_index < PCI_MAX_DEVICES;
             ++device_index) {
            uint8_t device = (uint8_t)device_index;
            uint32_t identity = config_read(bus, device, 0U, 0x00U);
            if ((uint16_t)identity == PCI_VENDOR_ABSENT) {
                continue;
            }

            uint32_t header_register = config_read(bus, device, 0U, 0x0CU);
            uint8_t function_count =
                (((header_register >> 16) & 0x80U) != 0U) ? PCI_MAX_FUNCTIONS : 1U;
            for (uint32_t function_index = 0U; function_index < function_count;
                 ++function_index) {
                uint8_t function = (uint8_t)function_index;
                if (function != 0U) {
                    identity = config_read(bus, device, function, 0x00U);
                    if ((uint16_t)identity == PCI_VENDOR_ABSENT) {
                        continue;
                    }
                    header_register = config_read(bus, device, function, 0x0CU);
                }

                if (device_count >= PCI_DEVICE_LIMIT) {
                    return false;
                }
                record_function(
                    bus, device, function, identity,
                    config_read(bus, device, function, 0x08U),
                    header_register,
                    config_read(bus, device, function, 0x3CU));
            }
        }
    }

    return true;
}

uint32_t pci_count(void) {
    return device_count;
}

const pci_device_t *pci_get(uint32_t index) {
    return index < device_count ? &devices[index] : 0;
}

const pci_device_t *pci_find(uint16_t vendor_id, uint16_t device_id) {
    for (uint32_t i = 0U; i < device_count; ++i) {
        if (devices[i].vendor_id == vendor_id && devices[i].device_id == device_id) {
            return &devices[i];
        }
    }
    return 0;
}
