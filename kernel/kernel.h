#ifndef MYOS_KERNEL_H
#define MYOS_KERNEL_H

#include <stdint.h>

#define MYOS_MULTIBOOT2_BOOT_MAGIC UINT32_C(0x36D76289)

void serial_initialize(void);
void serial_write(const char *text);
_Noreturn void panic(const char *message);
_Noreturn void kernel_main(uint32_t boot_magic, uint32_t boot_info_address);

#endif
