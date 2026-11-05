# Kernel notes

## Implemented

The Multiboot2 entry starts in 32-bit protected mode, verifies the boot magic, creates a temporary identity map for the first 1 GiB, enables x86_64 long mode, loads a minimal GDT, creates a stack, and calls the C entry point. The kernel initializes COM1 and legacy VGA text output, validates the handoff magic, logs status, and halts.

## Not implemented

The boot information pointer is not parsed. There is no IDT, exception/IRQ framework, timer, physical or virtual memory manager, heap, scheduler, process model, syscall entry, or user mode. Interrupts stay disabled. The bootstrap page tables are temporary and not a general virtual-memory implementation.

## Build assumptions

The compiler uses freestanding, no-PIE, no-stack-protector, no-red-zone, and no-SIMD flags. Linking uses the project's linker script and no standard libraries. `make check` verifies ELF64 format and the GRUB Multiboot2 header. QEMU/CI evidence must be checked before calling a boot test successful.
