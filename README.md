# MyOS

MyOS is an experimental independent x86_64 operating-system project written in freestanding C and NASM. It boots through GRUB Multiboot2 BIOS and currently focuses on kernel bring-up, memory/filesystem foundations, a small framebuffer desktop, and incremental networking work.

> **Status:** educational prototype. The main branch includes a booting desktop, polled input, memory-management primitives, RAM-only VFS, bounded PCI enumeration, and e1000 controller initialization. It is not a general-purpose OS or a production security appliance.

## Current capabilities

- GRUB Multiboot2 BIOS boot to a 64-bit kernel.
- Serial diagnostics, exception-only IDT, and QEMU boot smoke checks.
- A framebuffer desktop with four built-in themes and editable title; controls are runtime-only.
- Polled PS/2 keyboard and mouse input where emulated hardware is available.
- Initial physical-frame allocator, page mapping, and kernel heap foundations (not user isolation or SMP-safe).
- Volatile RAM filesystem API with boot-time self-tests; data is lost on reboot.
- Bounded PCI configuration-space enumeration and QEMU e1000 discovery/controller initialization.
- Generic network-device lifecycle/frame bounds and host-side tests using mock callbacks.
- Bounds-checked, read-only parsing of Ethernet, ARP-over-Ethernet/IPv4, and IPv4 headers for offline/test-buffer inspection.

## Networking: implemented vs. not implemented

The host parser and device registry are **foundations**, not network connectivity. The e1000 code reads controller state but does not configure DMA rings or send/receive packets. MyOS currently has no integrated packet path, DHCP, DNS, TCP/UDP services, browser, firewall, or Internet access. The parser is not a live capture tool and does not open host sockets.

Networking development is intentionally staged: establish safe DMA ownership and packet I/O in an isolated QEMU lab, test protocol parsers against synthetic and malformed frames, then add a small stack and controlled end-to-end tests. Network exercises should be limited to systems and lab targets the operator is authorized to test.

## Build and run

Supported development environments are Ubuntu/Debian x86_64 and GitHub Codespaces. The build toolchain includes GCC/binutils, Make, NASM, GRUB BIOS utilities, xorriso, mtools, QEMU, and GDB.

```sh
make setup
make clean
make kernel
make check
make iso
make run
```

Artifacts are written to `build/` (`kernel.elf` and `MyOS.iso`). `make debug` starts QEMU paused with its GDB stub on localhost:1234; attach with:

```sh
gdb build/kernel.elf -ex 'target remote :1234'
```

Host-only networking tests compile with native GCC and do not require root, a NIC, network access, or QEMU:

```sh
make test-network
```

The test target covers registry validation/lifecycle/frame bounds and parser behavior for valid, truncated, and malformed synthetic frames. It does not validate real NIC operation or protocols beyond the parsed headers.

## Architecture

```text
GRUB Multiboot2 BIOS
        |
        v
boot/boot.asm -> long mode -> kernel/main.c
                                  |-- memory/ + filesystem/ foundations
                                  |-- framebuffer desktop + polled input
                                  |-- drivers/pci/ -> e1000 controller initialization
                                  `-- drivers/net/ registry + bounded packet parsing
                                      (no live packet path yet)
```

See [architecture](docs/architecture.md), [kernel notes](docs/kernel.md), and the [roadmap](docs/roadmap.md).

## Roadmap

1. Run baseline build/ISO/QEMU checks and host networking tests.
2. Define and test DMA allocator/cache/descriptor ownership before implementing NIC packet I/O.
3. Connect one QEMU NIC to an isolated local test service; test frame TX/RX without external targets.
4. Add protocol layers incrementally with parser tests, then DHCP/DNS and controlled end-to-end connectivity.
5. Add explicit network policy, auditability, and defensive status tools before broader lab functionality.

## Contributing and safety

Keep changes incremental, testable, and truthful about completion. Do not treat source presence or device discovery as functional support. See [CONTRIBUTING.md](CONTRIBUTING.md), [Code of Conduct](CODE_OF_CONDUCT.md), and [Security Policy](SECURITY.md). Do not use this prototype for sensitive workloads.

Windows PE `.exe` support, user-mode processes, persistent storage, hardware interrupts/timers, full TCP/IP, physical NIC compatibility, and Internet access are not implemented.
