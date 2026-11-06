# MyOS development roadmap

This roadmap separates source code from integrated and verified behavior. A feature is considered verified only after the relevant build and runtime tests pass and evidence is recorded.

## Current milestone

- **Implemented boot path:** GRUB BIOS Multiboot2 entry, NASM transition to x86_64, freestanding C entry, early serial diagnostics, exception-only IDT, bounds-checked framebuffer and memory-map setup, physical 4 KiB frame allocation below 4 GiB, 4 KiB virtual mapping, a 64 MiB kernel heap, basic customizable desktop, polled PS/2 keyboard and mouse, and panic/halt path. Build and runtime status must be established by CI or local tests; this document does not assert a passing run.
- **Present but not integrated:** polled COM1 driver and serial shell command/input source; generic static network-device interface and registry. The current kernel entry does not wire these interfaces into the booted system.
- **Desktop prototype limits:** four volatile color themes, title editing, mouse pointer and clicks only; no saved preferences, windows, applications, or accelerated graphics. The interface requires a supported Multiboot2 framebuffer and PS/2-compatible input.
- **Implemented foundation only:** bounded PCI configuration-space enumeration over bus/device/function tuples; QEMU smoke test checks for the emulated e1000 function. No device is configured or driven by this enumerator.
- **Implemented device initialization only:** the QEMU e1000 is mapped uncached, reset, and queried for MAC and link state. DMA, Ethernet frame I/O, network protocols, and other NIC models are not implemented.
- **Implemented filesystem foundation:** path-based VFS API and volatile RAM filesystem with directory enumeration, metadata, create/read/write/copy/rename/remove operations; boot tests exercise operations and invalid/missing paths. No persistent storage or permissions.
- **Not implemented:** hardware IRQ routing/timers, per-process virtual address spaces and protection, NIC packet I/O, network protocols, block storage/persistent filesystems, desktop file explorer, processes/user mode, executable loader, and UEFI.

## Prioritized milestones

### 1. Establish a reproducible baseline

- Run `make clean`, `make kernel`, `make check`, and `make iso` in CI.
- Run a bounded QEMU boot smoke test that confirms framebuffer and desktop initialization, and retain serial logs/artifacts.
- Fix build/boot regressions before adding dependent subsystems.

### 2. Integrate and test the serial shell

- Wire serial and shell sources into the build and kernel entry.
- Exercise prompt, built-in commands, unknown command, backspace, line overflow, and halt in QEMU.
- Keep the shell explicitly a polled kernel-context COM1 interface; keyboard support for the shell and user processes remain separate work.

### 3. Add safe platform foundations

- Add allocation exhaustion and malformed-map tests; establish per-process address spaces and protection before user mode.
- Add hardware IRQ routing and timer support; keep maskable interrupts disabled until handlers and controllers are fully validated.
- Extend the bounded PCI enumeration tests to cover multi-function devices and table exhaustion; document configuration-space access.
- Define contiguous DMA allocation, ownership, synchronization, and teardown before enabling bus mastering.

### 4. Build networking in tested increments

- Integrate/test the generic network-device registry and cover invalid operations, registration bounds, start/stop, and frame limits.
- Complete e1000 support with bounded DMA allocation, ownership-safe TX/RX rings, and packet loopback tests before exposing it through the network-device registry.
- Add bounded Ethernet and protocol layers (begin with ARP and IPv4/ICMP) with explicit packet-buffer ownership.
- Add text adapter-management commands only after devices exist. Add more NIC drivers one at a time with supported IDs and tests.
- Physical NIC support and TCP/IP completeness are not implied.

### 5. Add OS services

- Memory manager and heap, then timer/scheduler.
- Add block storage and a persistent filesystem backend behind the VFS; define mount, error, permission, and timestamp semantics.
- Build the file explorer as a separate desktop application using only the VFS API after the application/window/input interfaces exist.
- User-mode process and syscall foundations.
- Decide a native executable format before implementing a loader. Windows PE `.exe` compatibility requires substantial PE loading, user-mode, ABI, and library compatibility and is not a near-term promise.
- Expand the framebuffer/keyboard/mouse desktop only after stable device and application interfaces; persistent settings require storage and a filesystem.
- Add a buffered rendering API, event-based input, timer, and user-mode application lifecycle before exposing a native game SDK or adding game applications.
- UEFI and broader hardware support as separately validated boot/platform targets.

## Release criteria

A release candidate requires clean reproducible builds, documented tested QEMU configurations, boot and subsystem regression tests, no generated artifacts in Git, a review of safety boundaries, and documentation that does not overstate physical-hardware or compatibility support.
