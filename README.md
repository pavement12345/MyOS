# MyOS

MyOS is an independent x86_64 operating-system project. Its kernel is original freestanding C and assembly; Linux is only the development host, not the finished system's kernel.

## Status

The repository is establishing a minimal GRUB Multiboot2 boot path. The current milestone is not a complete general-purpose OS: there is no scheduler, memory allocator, interrupt subsystem, disk filesystem, user mode, shell, graphical desktop, or physical-hardware support yet. Check GitHub Actions for actual build and QEMU smoke-test evidence.

## Architecture

- GRUB BIOS Multiboot2 loads the ELF64 kernel.
- NASM bootstrap code transitions from 32-bit protected mode into x86_64 long mode and calls freestanding C.
- The initial kernel reports boot status over COM1 and VGA text memory, then halts.
- Future drivers, MyFS/VFS, syscalls, shell, and desktop are planned but not operational.

See `docs/architecture.md`, `docs/kernel.md`, and `docs/roadmap.md`.

## Develop and build

In Codespaces, create a Codespace from this repository. On Ubuntu/Debian, run:

```sh
make setup
make check
make kernel
make iso
make run
```

The kernel output is `build/kernel.elf`; the ISO output is `build/MyOS.iso`. `make debug` starts QEMU with its GDB stub on localhost:1234. The build uses x86_64 host GCC in freestanding mode and does not link host libc; the setup check verifies GCC's target. A dedicated cross-compiler remains a future toolchain improvement.

The ISO currently targets GRUB BIOS boot. UEFI support is not implemented. QEMU is the target environment; no physical hardware or broad device compatibility is claimed. CI uploads an ISO artifact only when build and configured smoke tests pass.

## Limitations

At this milestone, no interrupt handling, dynamic memory management, storage, filesystem, keyboard/mouse input, process model, system calls, networking, or GUI is implemented. The bootstrap's 1 GiB identity map is temporary. Do not treat scaffold directories or planned interfaces as implemented features.
