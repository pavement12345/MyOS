# MyOS

MyOS is an independent x86_64 operating-system project. Its kernel is original freestanding C and assembly; Linux is only the development host, not the finished system's kernel.

## Status

The repository currently implements an initial GRUB Multiboot2 boot path and a small kernel boot smoke test. This is a bootable development milestone, **not** a complete general-purpose operating system. There is no scheduler, memory allocator, interrupt subsystem, disk filesystem, user mode, shell, graphical desktop, or physical-hardware support yet.

## Architecture

- GRUB loads the ELF64 kernel using Multiboot2.
- The kernel's NASM entry switches from the bootloader's 32-bit protected mode to x86_64 long mode, establishes temporary identity-mapped paging and a stack, then calls freestanding C.
- The initial kernel reports boot status over COM1 serial and VGA text memory. It halts rather than claiming unsupported services.
- Future kernel components will expose explicit interfaces for drivers, MyFS/VFS, system calls, shell, and eventually desktop applications.

See `docs/architecture.md`, `docs/kernel.md`, and `docs/roadmap.md` for implemented behavior and limitations.

## Development environment

On Ubuntu/Debian or in the provided Codespaces dev container:

```sh
make setup
make check
```

The current build uses the host GCC only after verifying that it targets x86_64, then links freestanding code as a static ELF kernel. It does not link host libc. Phase 2's build-system decision is documented; a dedicated `x86_64-elf` cross-toolchain can replace the validated host-target compiler when needed.

## Build and run

```sh
make kernel      # build/kernel.elf
make iso         # build/MyOS.iso
make check       # build and validate the Multiboot2 kernel
make run         # boot the ISO in QEMU
make debug       # start QEMU stopped with a GDB server on TCP :1234
make clean
```

To attach GDB in another terminal after `make debug`, use:

```sh
gdb build/kernel.elf
(gdb) target remote :1234
```

The ISO is a GRUB BIOS boot ISO. UEFI boot support is not implemented. QEMU boot testing is the target; no physical machine or broad device compatibility is claimed. The CI workflow builds and uploads `build/MyOS.iso` as an artifact; it does not publish releases.

## Repository structure

The source follows the planned directory layout. `linker.ld` is at the repository root because it is a build/linker configuration, and `.github/workflows/build-iso.yml` automates build and smoke-test validation.

## Limitations

At this milestone, MyOS only initializes enough CPU state to enter 64-bit C code, writes a startup message, validates the Multiboot2 handoff magic, and halts. Temporary page tables identity-map the first 1 GiB using 2 MiB pages. No interrupt handling, dynamic memory management, storage, filesystem, input, process model, syscalls, networking, or GUI is implemented. A serial boot smoke test is not evidence that these future features work.
