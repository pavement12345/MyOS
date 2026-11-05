#include "kernel.h"

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void serial_putc(char c) {
    while ((inb(0x3FD) & 0x20U) == 0U) __asm__ volatile ("pause");
    outb(0x3F8, (uint8_t)c);
}

void serial_initialize(void) {
    outb(0x3F9, 0x00); outb(0x3FB, 0x80); outb(0x3F8, 0x03);
    outb(0x3F9, 0x00); outb(0x3FB, 0x03); outb(0x3FA, 0xC7);
    outb(0x3FC, 0x0B);
}

void serial_write(const char *text) {
    if (text == 0) return;
    while (*text != '\0') {
        if (*text == '\n') serial_putc('\r');
        serial_putc(*text++);
    }
}

static void vga_write(const char *text) {
    volatile uint16_t *video = (volatile uint16_t *)0xB8000;
    for (uint32_t i = 0; text[i] && i < 80U * 25U; ++i)
        video[i] = (uint16_t)(0x0FU << 8) | (uint8_t)text[i];
}

_Noreturn void kernel_main(uint32_t boot_magic, uint32_t boot_info_address) {
    (void)boot_info_address;
    serial_initialize();
    serial_write("MyOS: entered x86_64 kernel\n");
    if (boot_magic != MYOS_MULTIBOOT2_BOOT_MAGIC) panic("invalid Multiboot2 handoff magic");
    vga_write("MyOS kernel booted - COM1 serial active");
    serial_write("MyOS: Multiboot2 handoff validated\n");
    serial_write("MyOS: boot milestone only; other OS services are not implemented\n");
    for (;;) __asm__ volatile ("cli; hlt");
}
