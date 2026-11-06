#ifndef MYOS_E1000_H
#define MYOS_E1000_H

#include "pci.h"

#include <stdbool.h>
#include <stdint.h>

bool e1000_initialize(const pci_device_t *device);
bool e1000_is_initialized(void);
bool e1000_link_is_up(void);
const uint8_t *e1000_mac_address(void);

#endif
