# MyOS development roadmap

This roadmap separates source code from integrated and verified behavior. A feature is considered verified only after the relevant build and runtime tests pass and evidence is recorded.

## Current milestone

- **Implemented boot path:** GRUB BIOS Multiboot2 entry, NASM transition to x86_64, freestanding C entry, early serial/VGA diagnostics, panic/halt path. Current build/boot status must be established by CI; this document does not assert a passing run.
- **Present but not integrated:** polled COM1 driver and serial shell command/input source; generic static network-device interface and registry. The current Makefile/kernel entry do not wire these components into the booted system.
- **Not implemented:** PCI discovery, physical/virtual memory managers, IDT/interrupt framework, NIC drivers, network protocols, storage/filesystems, processes/user mode, executable loader, graphics, and UEFI.

## Prioritized milestones

### 1. Establish a reproducible baseline

- Run `make clean`, `make kernel`, `make check`, and `make iso` in CI.
- Run a bounded QEMU boot smoke test and retain serial logs/artifacts.
- Fix build/boot regressions before adding dependent subsystems.

### 2. Integrate and test the serial shell

- Wire serial and shell sources into the build and kernel entry.
- Exercise prompt, built-in commands, unknown command, backspace, line overflow, and halt in QEMU.
- Keep the interface explicitly a polled kernel-context COM1 shell; keyboard input and user processes are separate future work.

### 3. Add safe platform foundations

- Parse and bounds-check Multiboot2 boot information and memory map.
- Add IDT and exception diagnostics; keep interrupts disabled until full setup is valid.
- Add PCI enumeration and document configuration-space access.
- Define memory allocation, MMIO mapping, DMA ownership, and teardown rules before bus-mastering drivers.

### 4. Build networking in tested increments

- Integrate/test the generic network-device registry and cover invalid operations, registration bounds, start/stop, and frame limits.
- Implement one documented QEMU NIC only after PCI and memory/DMA foundations; test link and packet I/O in QEMU.
- Add bounded Ethernet and protocol layers (begin with ARP and IPv4/ICMP) with explicit packet-buffer ownership.
- Add text adapter-management commands only after devices exist. Add more NIC drivers one at a time with supported IDs and tests.
- Physical NIC support, TCP/IP completeness, DHCP, and a graphical settings UI are not implied.

### 5. Add OS services

- Memory manager and heap, then timer/scheduler.
- Block storage, filesystem, and VFS.
- User-mode process and syscall foundations.
- Decide a native executable format before implementing a loader. Windows PE `.exe` compatibility requires substantial PE loading, user-mode, ABI, and library compatibility and is not a near-term promise.
- Keyboard, graphics, and a desktop only after stable device and application interfaces.
- UEFI and broader hardware support as separately validated boot/platform targets.

## Release criteria

A release candidate requires clean reproducible builds, documented tested QEMU configurations, boot and subsystem regression tests, no generated artifacts in Git, a review of safety boundaries, and documentation that does not overstate physical-hardware or compatibility support.
