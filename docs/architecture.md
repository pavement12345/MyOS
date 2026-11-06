# Architecture

## Current boot milestone

GRUB's Multiboot2 BIOS loader loads the ELF64 kernel, requests an 800x600x32 framebuffer, and transfers control to `_start` in 32-bit protected mode. NASM bootstrap code checks the Multiboot2 magic, creates temporary identity-mapped page tables for the first 4 GiB using 2 MiB pages, enables PAE and long mode, loads a minimal GDT, sets a kernel stack, and calls the freestanding C entry. The C kernel bounds-checks the Multiboot2 information, framebuffer tag, and memory map; initializes COM1 and a physical frame allocator; draws a full-screen desktop; and polls PS/2 keyboard and mouse input.

The physical allocator manages 4 KiB frames below 4 GiB, frees only usable ranges from the bootloader map, and reserves low memory, the kernel, boot information, and framebuffer. Four-level paging can map/unmap individual 4 KiB pages in lower-canonical virtual addresses using physical frames below 4 GiB. A growable 64 MiB kernel heap uses that mapping API and coalesces freed blocks. These primitives do not yet provide per-process address spaces, memory protection, or synchronization.

An IDT handles CPU exceptions, reports unexpected faults, and is smoke-tested by recovering from a breakpoint and a controlled page fault. Hardware interrupts remain disabled.

PCI configuration mechanism #1 (I/O ports 0xCF8/0xCFC) enumerates bus/device/function tuples and records vendor/device IDs and basic class/header/interrupt-line fields in a fixed-size table. QEMU smoke testing verifies discovery of its emulated Intel e1000 function. The e1000 driver supports only QEMU's 8086:100e device: it enables PCI memory-space access, marks the first 24 KiB of its MMIO BAR uncached, resets the controller, and reads its MAC and link state. It does not enable bus mastering, configure DMA rings, transmit or receive packets, or route interrupts.

The desktop accepts 32-bit RGB framebuffers from 640x480 through 1920x1080 and provides four in-memory color themes plus an editable title. Keyboard and mouse input use polling and do not enable interrupts. If PS/2 mouse initialization fails, keyboard controls remain available. Settings are not persisted. The system still lacks per-process address spaces, hardware IRQ routing, timers, storage, applications, a window manager, and UEFI boot.

The filesystem layer exposes a path-based VFS API in `filesystem/vfs.h`; its first backend is a RAM-only filesystem. It supports directory enumeration, metadata (name/type/size), file creation, bounded read/write, file copy, rename/move, and removal of files and empty directories. Boot self-tests exercise these operations against temporary test files and remove them before entering the desktop. Contents disappear on reboot: no block driver, persistent filesystem, mount interface, timestamps, permission checks, or file handles are present. Desktop applications must use the VFS API rather than kernel memory or device internals.

## Intended subsystem boundaries

The planned kernel is initially monolithic, with separate CPU, interrupt, memory, scheduler, driver, filesystem, syscall, and shell components. Drivers will provide kernel-facing device interfaces; VFS will decouple filesystem clients from filesystem implementations; user interfaces will use explicit kernel APIs. The frame allocator, basic page mapper, kernel heap, PCI configuration-space enumeration, framebuffer, polled PS/2 input, and RAM-only VFS backend are initial low-level pieces only; these broader interfaces are not yet implemented.

## Build and validation

The Makefile builds freestanding ELF64 objects without linking host libc. GRUB utilities inspect the Multiboot2 header; xorriso creates the GRUB BIOS ISO. The Actions workflow runs a bounded QEMU framebuffer/desktop boot smoke test before uploading the ISO artifact. A passing test validates only the tested boot path in that environment, not UEFI, physical hardware, interactive input, or unfinished features.
