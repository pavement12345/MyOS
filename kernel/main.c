#include "kernel.h"
#include "desktop.h"
#include "framebuffer.h"
#include "interrupts.h"
#include "mouse.h"
#include "physical.h"
#include "virtual.h"
#include "heap.h"
#include "pci.h"
#include "e1000.h"
#include "selftest.h"

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
    if (!interrupts_initialize()) {
        panic("IDT exception self-test failed");
    }
    serial_write("MyOS: IDT loaded; exception path self-test passed\n");
    if (!pci_initialize()) {
        panic("PCI enumeration exceeded its bounded device table");
    }
    serial_write("MyOS: PCI enumeration complete\n");
    if (pci_find(0x8086U, 0x100EU) != 0) {
        serial_write("MyOS: QEMU e1000 PCI function discovered\n");
    }

    framebuffer_t framebuffer;
    if (!framebuffer_initialize(boot_info_address, &framebuffer)) {
        panic("no supported Multiboot2 RGB framebuffer (need 640x480 to 1920x1080, 32 bpp)");
    }
    serial_write("MyOS: framebuffer initialized\n");
    uint32_t available_frames;
    if (!physical_memory_initialize(boot_info_address, &framebuffer, &available_frames)) {
        panic("invalid Multiboot2 memory map or no usable physical frames");
    }
    const pci_device_t *network_device = pci_find(0x8086U, 0x100EU);
    if (network_device != 0) {
        if (!e1000_initialize(network_device)) {
            panic("QEMU e1000 MMIO initialization failed");
        }
        serial_write("MyOS: e1000 reset; MAC address read from controller\n");
        serial_write(e1000_link_is_up() ? "MyOS: e1000 link is up\n" :
                                         "MyOS: e1000 link is down\n");
    }
    uint64_t first_frame;
    uint64_t second_frame;
    uint64_t released_frame;
    if (!physical_frame_allocate(&first_frame) ||
        !physical_frame_allocate(&second_frame) ||
        first_frame == second_frame ||
        !physical_frame_release(first_frame) ||
        !physical_frame_allocate(&released_frame) ||
        released_frame != first_frame ||
        !physical_frame_release(released_frame) ||
        !physical_frame_release(second_frame)) {
        panic("physical frame allocator self-test failed");
    }
    serial_write("MyOS: Multiboot2 memory map validated; physical frame allocator ready\n");
    uint64_t mapped_frame;
    uint64_t unmapped_frame;
    volatile uint64_t *test_mapping = (volatile uint64_t *)(uintptr_t)UINT64_C(0x1000000000);
    if (!physical_frame_allocate(&mapped_frame) ||
        !virtual_memory_map_page((uint64_t)(uintptr_t)test_mapping, mapped_frame)) {
        panic("virtual page map self-test setup failed");
    }
    if (virtual_memory_map_page((uint64_t)(uintptr_t)test_mapping, mapped_frame) ||
        virtual_memory_unmap_page((uint64_t)(uintptr_t)test_mapping + 1U,
                                  &unmapped_frame)) {
        panic("virtual page map accepted a duplicate or unaligned operation");
    }
    test_mapping[0] = UINT64_C(0x4D794F5350414745);
    test_mapping[511] = UINT64_C(0x5445535450414745);
    if (test_mapping[0] != UINT64_C(0x4D794F5350414745) ||
        test_mapping[511] != UINT64_C(0x5445535450414745) ||
        !virtual_memory_unmap_page((uint64_t)(uintptr_t)test_mapping, &unmapped_frame) ||
        unmapped_frame != mapped_frame ||
        !physical_frame_release(unmapped_frame)) {
        panic("virtual page map/unmap self-test failed");
    }
    serial_write("MyOS: virtual page map/unmap self-test passed\n");
    if (kernel_heap_allocate(0U) != 0 ||
        kernel_heap_allocate(SIZE_MAX) != 0) {
        panic("kernel heap accepted an invalid allocation size");
    }
    uint8_t *heap_small = (uint8_t *)kernel_heap_allocate(37U);
    uint8_t *heap_large = (uint8_t *)kernel_heap_allocate(5000U);
    if (heap_small == 0 || heap_large == 0) {
        panic("kernel heap self-test allocation failed");
    }
    heap_small[0] = 0x5AU;
    heap_small[36] = 0xA5U;
    heap_large[0] = 0x3CU;
    heap_large[4999] = 0xC3U;
    if (heap_small[0] != 0x5AU || heap_small[36] != 0xA5U ||
        heap_large[0] != 0x3CU || heap_large[4999] != 0xC3U ||
        !kernel_heap_release(heap_small) || !kernel_heap_release(heap_large) ||
        kernel_heap_release(heap_large)) {
        panic("kernel heap self-test failed");
    }
    serial_write("MyOS: kernel heap allocation/coalescing self-test passed\n");
    if (!filesystem_self_test()) {
        panic("RAM filesystem API self-test failed");
    }
    serial_write("MyOS: RAM filesystem operations self-test passed\n");
    bool mouse_available = mouse_initialize(framebuffer.width, framebuffer.height);
    serial_write(mouse_available ? "MyOS: PS/2 mouse initialized\n" :
                                   "MyOS: PS/2 mouse unavailable; keyboard controls remain active\n");
    serial_write("MyOS: desktop ready; T/E keys, mouse theme and title controls\n");
    desktop_run(&framebuffer, mouse_available);
}
