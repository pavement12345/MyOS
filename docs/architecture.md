# Architecture

## Current boot milestone

GRUB's Multiboot2 BIOS loader loads the ELF64 kernel and transfers control to `_start` in 32-bit protected mode. NASM bootstrap code checks the Multiboot2 magic, creates temporary identity-mapped page tables for the first 1 GiB using 2 MiB pages, enables PAE and long mode, loads a minimal GDT, sets a kernel stack, and calls the freestanding C entry. The C kernel initializes COM1, checks the handoff magic, writes a VGA text message, and halts.

This is a minimal boot demonstration. It does not parse the Multiboot2 memory map or implement interrupts, memory allocation, storage, user mode, or desktop support. UEFI boot is not implemented.

## Intended subsystem boundaries

The planned kernel is initially monolithic, with separate CPU, interrupt, memory, scheduler, driver, filesystem, syscall, and shell components. Drivers will provide kernel-facing device interfaces; VFS will decouple filesystem clients from MyFS; user interfaces will use explicit kernel APIs. These interfaces are not yet implemented.

## Build and validation

The Makefile builds freestanding ELF64 objects without linking host libc. GRUB utilities inspect the Multiboot2 header; xorriso creates the GRUB BIOS ISO. The Actions workflow runs a QEMU serial boot smoke test before uploading the ISO artifact. A passing test validates only the tested boot path in that environment, not UEFI, physical hardware, or unfinished features.
