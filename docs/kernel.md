# Kernel notes

## Implemented

The Multiboot2 entry starts in 32-bit protected mode, verifies the boot magic, creates a temporary identity map for the first 4 GiB, enables x86_64 long mode, loads a minimal GDT, creates a stack, and calls the C entry point. The kernel installs an IDT with CPU exception stubs and verifies breakpoint return and page-fault recovery. It initializes COM1, validates the handoff magic, bounds-checks Multiboot2 tags, accepts only a 32-bit RGB framebuffer with supported dimensions and non-overlapping color channels, and validates the memory-map entries. A bitmap physical allocator manages 4 KiB usable frames below 4 GiB and reserves low memory, the kernel image, boot information, and framebuffer. Boot runs an allocate/release/reallocate check before starting the desktop.

Four-level paging maps and unmaps individual 4 KiB pages using allocated frames. The kernel heap grows in 64 KiB increments up to 64 MiB and provides aligned allocations, block splitting/coalescing, and invalid/double-free rejection. It is a single-kernel heap without synchronization or user address-space isolation.

The desktop has four color themes and an editable title; values are volatile and reset on reboot. A PS/2 mouse can move the drawn pointer, select a theme swatch, and focus the title field; if mouse initialization fails, keyboard controls still work. It is a QEMU-oriented first UI, not a window manager or application environment.

## Not implemented

There is no hardware IRQ routing, timer, scheduler, process model, syscall entry, or user mode. Maskable interrupts stay disabled; the IDT currently provides exception paths only. Physical frames are limited to the first 4 GiB. Paging/heap support does not yet provide per-process address spaces, privilege isolation, reclamation of heap pages, or SMP synchronization. The bootstrap page tables identity-map the first 4 GiB and are not a complete virtual-memory design. There is no persistent settings storage, graphics acceleration, or physical-hardware compatibility claim.

## Build assumptions

The compiler uses freestanding, no-PIE, no-stack-protector, no-red-zone, and no-SIMD flags. Linking uses the project's linker script and no standard libraries. `make check` verifies ELF64 format and the GRUB Multiboot2 header. QEMU/CI evidence must be checked before calling a boot test successful.
