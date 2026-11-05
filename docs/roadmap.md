# Roadmap

Status is based on implemented and verified source. No future feature should be marked complete until it is built and tested.

## Implemented in source; CI verification pending

- Phase 1: repository baseline, Codespaces config, host setup script.
- Phase 2: freestanding compiler flags, linker script, separate C/NASM Make rules.
- Phase 3: GRUB Multiboot2 configuration.
- Phase 4: original assembly-to-C x86_64 startup, serial logging, handoff magic check, VGA text output, panic/halt path.
- Phase 5: GRUB BIOS ISO creation and artifact workflow.
- Phase 6: QEMU serial boot smoke test in CI.

## Not implemented

- Phase 7: full GDT/IDT/exception/IRQ subsystem.
- Phase 8: physical/virtual memory managers, paging API, heap.
- Phase 9: framebuffer graphics and keyboard/mouse drivers.
- Phase 10: hardware timer, threads, and scheduler.
- Phase 11: block storage, MyFS format, and VFS.
- Phase 12: user-mode processes and system calls.
- Phase 13: functional MyShell and filesystem commands.
- Phase 14: reusable framebuffer graphics.
- Phase 15: MyDesktop/window manager.
- Phase 16: applications and networking.
- Phase 17: stability, broader testing, optimization.
- Phase 18: release-ready ISO and UEFI support.

## Limitations

The boot code identity maps only the first GiB temporarily, uses a BIOS Multiboot2 path, and assumes low memory placement. It does not consume boot memory-map information. No physical-hardware support is claimed. A successful CI run is required before claiming the boot milestone verified.
