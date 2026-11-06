#include "kernel.h"
#include "desktop.h"
#include "framebuffer.h"

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

_Noreturn void kernel_main(uint32_t boot_magic, uint32_t boot_info_address) {
    serial_initialize();
    serial_write("MyOS: entered x86_64 kernel\n");
    if (boot_magic != MYOS_MULTIBOOT2_BOOT_MAGIC) panic("invalid Multiboot2 handoff magic");
    serial_write("MyOS: Multiboot2 handoff validated\n");

    framebuffer_t framebuffer;
    if (!framebuffer_initialize(boot_info_address, &framebuffer)) {
        panic("no supported Multiboot2 RGB framebuffer (need 640x480 to 1920x1080, 32 bpp)");
    }
    serial_write("MyOS: framebuffer initialized\n");
    serial_write("MyOS: desktop ready; press T to change theme or E to edit title\n");
    desktop_run(&framebuffer);
}
