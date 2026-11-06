#include "interrupts.h"
#include "kernel.h"

#define IDT_ENTRY_COUNT 256U

typedef struct __attribute__((packed)) {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t attributes;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} idt_gate_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} idt_pointer_t;

extern void (*isr_stub_table[32])(void);
extern void isr_unhandled(void);
extern void exception_page_fault_test(void);
extern void page_fault_test_load(void);
extern void page_fault_test_recovery(void);

static idt_gate_t idt[IDT_ENTRY_COUNT];
static volatile uint32_t breakpoint_count;
static volatile uint32_t page_fault_count;
static bool page_fault_test_active;

static void set_gate(uint32_t vector, void (*handler)(void)) {
    uint64_t address = (uint64_t)(uintptr_t)handler;
    idt[vector].offset_low = (uint16_t)address;
    idt[vector].selector = 0x18U;
    idt[vector].ist = 0U;
    idt[vector].attributes = 0x8EU;
    idt[vector].offset_middle = (uint16_t)(address >> 16);
    idt[vector].offset_high = (uint32_t)(address >> 32);
    idt[vector].reserved = 0U;
}

bool interrupts_initialize(void) {
    for (uint32_t vector = 0U; vector < IDT_ENTRY_COUNT; ++vector) {
        set_gate(vector, isr_unhandled);
    }
    for (uint32_t vector = 0U; vector < 32U; ++vector) {
        set_gate(vector, isr_stub_table[vector]);
    }

    idt_pointer_t pointer = {
        .limit = (uint16_t)(sizeof(idt) - 1U),
        .base = (uint64_t)(uintptr_t)idt
    };
    __asm__ volatile ("lidt %0" : : "m"(pointer));

    breakpoint_count = 0U;
    __asm__ volatile ("int3");
    page_fault_count = 0U;
    page_fault_test_active = true;
    exception_page_fault_test();
    page_fault_test_active = false;
    if (breakpoint_count != 1U || page_fault_count != 1U) {
        return false;
    }
    return true;
}

void interrupt_exception_handler(uint64_t vector, uint64_t error_code,
                                 uint64_t *instruction_pointer) {
    if (vector == 3U) {
        ++breakpoint_count;
        return;
    }
    if (vector == 14U && page_fault_test_active &&
        *instruction_pointer == (uint64_t)(uintptr_t)page_fault_test_load &&
        (error_code & 0x07U) == 0U) {
        ++page_fault_count;
        *instruction_pointer = (uint64_t)(uintptr_t)page_fault_test_recovery;
        return;
    }

    static const char *const names[32] = {
        "divide error", "debug", "non-maskable interrupt", "breakpoint",
        "overflow", "bound range", "invalid opcode", "device unavailable",
        "double fault", "coprocessor segment overrun", "invalid TSS",
        "segment not present", "stack-segment fault", "general protection",
        "page fault", "reserved exception", "x87 floating point",
        "alignment check", "machine check", "SIMD floating point",
        "virtualization", "control protection", "reserved exception",
        "reserved exception", "reserved exception", "reserved exception",
        "reserved exception", "reserved exception", "hypervisor injection",
        "VMM communication", "security exception", "reserved exception"
    };
    serial_write("MyOS CPU exception: ");
    serial_write(vector < 32U ? names[vector] : "unexpected interrupt");
    serial_write("\n");
    panic("CPU exception handler halted the kernel");
}
