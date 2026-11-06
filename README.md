![Project Banner](assets/banner.png)
# MyOS

**MyOS is an independent x86_64 operating-system development project.** The kernel is written in freestanding C and assembly. Linux is a development host only; MyOS is not a Linux distribution.

> **Status:** experimental, BIOS/GRUB Multiboot2 boot path. The kernel boots a basic framebuffer desktop with polled keyboard and PS/2 mouse controls, plus COM1 diagnostics. It is not a general-purpose OS; settings are runtime-only, and the serial shell and generic network-device registry are not integrated into the boot path.

## What exists today

- GRUB BIOS Multiboot2 entry and NASM transition to 64-bit long mode.
- Freestanding C kernel entry, serial diagnostics, a Multiboot2 RGB framebuffer desktop, and a panic/halt path.
- An IDT with CPU exception reporting, tested by recovering from a breakpoint and a controlled page fault.
- Polled PS/2 keyboard and mouse input for cycling four color themes, editing the desktop title, moving a pointer, and clicking theme/title controls.
- A bounds-checked Multiboot2 memory-map parser and a physical 4 KiB frame allocator for the identity-mapped first 4 GiB, with kernel, boot-information, and framebuffer memory reserved.
- Kernel 4 KiB virtual page mapping and a 64 MiB growable heap with aligned allocation, splitting, and coalescing.
- Bounded PCI configuration-space enumeration; the QEMU boot smoke test verifies discovery of the emulated e1000 function. This is discovery only, not NIC or storage support.
- Initial QEMU e1000 initialization reads the controller's MAC and link state after an uncached MMIO mapping and reset. It does not yet transmit or receive frames.
- A path-based VFS API backed by a volatile RAM filesystem, with directory listing, name/type/size metadata, file and directory creation, file reads/writes, file copy, rename/move, and safe removal of files or empty directories. Boot tests exercise these operations and clean up their test data.
- Build scripts for a kernel ELF and GRUB BIOS ISO.
- Early COM1 shell and generic network-device registry source files are present on the development branch, but the current Makefile and kernel entry do not yet include them. They must not be treated as usable features.

The desktop requires a Multiboot2-provided 32-bit RGB framebuffer (640x480 through 1920x1080) and PS/2-compatible input devices. Press **T** to cycle Midnight, Ocean, Forest, and Ember themes; press **E** to enter a title, type it, and press Enter to apply or Escape to cancel. With a mouse, click a theme swatch to select it or click the title field and type. These settings are held in memory and reset on reboot. This is an early full-screen UI, not a window manager: the RAM filesystem is volatile and not yet exposed through an explorer; applications, networking, downloads, persistent storage/settings, user accounts, and graphics acceleration are not implemented.

The early heap uses a fixed 64 MiB virtual range and currently has no synchronization or user/kernel protection; maskable interrupts remain disabled. It is not a user-space allocator.

Not yet implemented as a verified subsystem: hardware IRQ routing and timers, per-process address spaces and memory protection, e1000 DMA and packet I/O, network protocols, block storage and persistent filesystems, file explorer, application lifecycle, user mode, executable loading, game runtime, UEFI, and Windows PE `.exe` compatibility. The RAM filesystem and memory primitives are kernel facilities only and are not sufficient to run applications or drivers.

## Architecture

```text
GRUB BIOS / Multiboot2
        |
        v
boot/boot.asm -- 32-bit entry, temporary identity map, long-mode transition
        |
        v
kernel/main.c -- freestanding kernel entry and early diagnostics
        |                     |                     |
        v                     v                     v
COM1 serial          framebuffer desktop     polled PS/2 keyboard

Planned, not yet integrated: memory/ -> filesystem/ -> shell/ -> user applications
```

See [architecture](docs/architecture.md), [kernel notes](docs/kernel.md), and the [roadmap](docs/roadmap.md) for boundaries and limitations.

## Prerequisites & dependencies

The supported development host is Ubuntu/Debian x86_64 or GitHub Codespaces. `make setup` installs the required packages (using `sudo` unless already root):

- GCC targeting x86_64, GNU binutils/linker, GNU Make, and NASM.
- GRUB utilities (`grub-file`, `grub-mkrescue`) and GRUB BIOS modules.
- xorriso and mtools for ISO creation.
- QEMU x86_64 for virtual-machine testing.
- GDB, Python 3, Git, `file`, and standard shell utilities for checks/debugging.

Install and check tools:

```sh
make setup
bash scripts/check.sh
```

No external kernel headers, host libc, `.env` file, or runtime service configuration is required. The kernel is freestanding and does not link host libc. Do not create configuration values or secrets for features that do not exist.

## Build and run

```sh
make clean
make kernel
make check
make iso
make run
```

Outputs are kept under `build/` and are not source files:

- `build/kernel.elf` — ELF64 kernel checked for the expected boot format.
- `build/MyOS.iso` — GRUB BIOS ISO.

`make run` launches QEMU and attaches its serial console to the terminal. `make debug` starts QEMU paused with a GDB stub on `localhost:1234`; in another terminal, attach using:

```sh
gdb build/kernel.elf -ex 'target remote :1234'
```

QEMU is the project’s current target environment. Hardware coverage, UEFI boot, and physical-device compatibility are not claimed. A successful build or CI boot smoke test does not imply that planned subsystems work.

## Development quality gates

Before submitting a change, run the checks relevant to it:

```sh
make clean
make kernel
make check
make iso
```

For boot-path changes, run the ISO in QEMU and inspect serial output and the framebuffer. The GitHub Actions workflow builds/validates the kernel and ISO and performs a bounded QEMU boot smoke test that checks framebuffer and desktop initialization. It does not yet test interactive keyboard controls, shell commands, or network traffic. See [Contributing](CONTRIBUTING.md) and [security reporting](SECURITY.md).

## Contributing

Keep changes small and subsystem-focused. Preserve the established directory layout, validate boot-provided data before use, keep interrupts disabled until exception/interrupt setup is sound, and document unimplemented behavior plainly. Do not add success-only stubs or claim a driver, executable format, or feature is supported without tests. See [CONTRIBUTING.md](CONTRIBUTING.md) and the [Code of Conduct](CODE_OF_CONDUCT.md).

## License

No license is declared in this README. Check the repository’s license metadata before redistributing or reusing the source.
