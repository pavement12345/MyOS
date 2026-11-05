# MyOS

MyOS is an independent operating-system project targeting x86_64. Its kernel will be written from original source in freestanding C and assembly. Linux is used only as a development host; MyOS is not a Linux distribution.

## Status

**Current phase: Phase 1 — development environment and repository setup.**

The kernel, boot process, filesystem, shell, desktop, and bootable ISO have not been implemented yet. Build targets for these features will be enabled as their prerequisites are developed and tested.

## Planned architecture

- **Boot:** GRUB will initially load the kernel and pass boot information through a defined protocol.
- **Kernel:** An independently written, initially monolithic kernel with modular CPU, interrupt, memory, scheduling, driver, filesystem, and syscall components.
- **Hardware:** QEMU virtual hardware is the initial target. Physical hardware support will only be claimed after testing.
- **Filesystem:** MyFS will use an original documented disk format, exposed through a VFS.
- **User interfaces:** MyShell and, later, MyDesktop will depend on kernel services that must be implemented first.
- **Build:** A validated freestanding x86_64 toolchain, Make, scripts, and GitHub Actions will produce and validate build artifacts.

## Development requirements

The setup script installs host tools including Make, GCC, GNU Binutils, NASM, GRUB utilities, xorriso, QEMU, GDB, Git, and Python 3.

The host GCC is **not** assumed to be suitable for kernel compilation. A freestanding x86_64 cross-compiler is a Phase 2 task.

## Development environment

In GitHub Codespaces, create a Codespace from this repository. The dev container runs `scripts/setup.sh` after creation.

For an Ubuntu or Debian host with `sudo`, run:

```bash
make setup
make check
```

## Current build commands

```bash
make help
make check
make clean
```

The `kernel`, `iso`, `run`, and `debug` targets are intentionally not implemented yet. They will be enabled as the project reaches those phases.

## ISO and QEMU

No ISO exists yet. Once the kernel and ISO generation are implemented, the intended output is `build/MyOS.iso`, and QEMU will be the initial boot-test environment. A successful build or boot will only be reported after it has actually been run and verified.

## Documentation

- `docs/architecture.md` — overall architecture and component boundaries
- `docs/kernel.md` — kernel initialization and subsystem design
- `docs/roadmap.md` — completed, in-progress, and planned work

## Limitations

At this phase, MyOS has no kernel, bootloader configuration, drivers, filesystem, shell, graphical interface, or bootable ISO. Host-tool availability does not imply support for any hardware or OS feature.
