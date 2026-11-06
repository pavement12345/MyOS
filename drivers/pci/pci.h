#ifndef MYOS_PCI_H
#define MYOS_PCI_H

#include <stdbool.h>
#include <stdint.h>

#define PCI_DEVICE_LIMIT 256U

typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint8_t header_type;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t programming_interface;
    uint8_t interrupt_line;
    uint16_t vendor_id;
    uint16_t device_id;
} pci_device_t;

bool pci_initialize(void);
uint32_t pci_count(void);
const pci_device_t *pci_get(uint32_t index);
const pci_device_t *pci_find(uint16_t vendor_id, uint16_t device_id);

#endif
