# MyOS agent and contributor instructions

## Project goal and truthfulness
- MyOS is an independently written x86_64 OS kernel. Linux is a development host only; do not turn this into a Linux distribution.
- Work incrementally. Never mark a subsystem complete, claim device support, or report a successful build/boot without actual test evidence.
- Do not add placeholder implementations that return success or print success for functionality they do not perform. Document unfinished work explicitly.
- Keep generated objects, ELF binaries, disk images, and ISOs out of Git; build output belongs under `build/`.

## Preserve project layout
- Follow the directory layout in the project brief. Do not rename or relocate established source directories.
- Put architecture-specific boot code in `boot/`, kernel subsystems in `kernel/`, memory code in `memory/`, drivers in `drivers/`, filesystem code in `filesystem/`, build utilities in `tools/` or `scripts/`, and documentation in `docs/`.
- Keep the root linker script as `linker.ld` unless there is a concrete, documented reason to change it.
- Add new files only when needed and explain their role in documentation or the change summary.

## Kernel implementation rules
- Kernel code is freestanding C/assembly. Do not include host libc or link host runtime libraries into the kernel.
- Keep compiler flags architecture-appropriate and compile C and NASM separately. Preserve dependency generation where applicable.
- Respect the boot protocol's register and memory contract. Validate boot-provided structures and bounds before using them.
- Keep interrupts disabled until the IDT, exception paths, and interrupt controller setup are valid.
- Do not add scheduling, heap allocation, user mode, filesystem operations, or graphics claims before their required foundations exist.
- Keep C++ desktop components separate from kernel code and do not imply they run until an explicit kernel/application interface exists.

## Changes and verification
1. Inspect current source and documentation before editing; avoid replacing unrelated work.
2. Make the smallest coherent change for the current phase.
3. Run relevant checks. For build changes, use `make clean`, `make kernel`, `make check`, and `make iso` when dependencies are available. For boot changes, run the ISO in QEMU and verify serial output.
4. If a check cannot be run, say so plainly and provide the exact command needed. Do not report it as passing.
5. Update `README.md` and `docs/roadmap.md` when implemented behavior or limitations change.
6. Review the diff for generated files, unsafe assumptions, and claims that exceed tested behavior.

## Agent scope
- An agent may implement a bounded subsystem or review a defined change, but must not assume another agent's work is merged or tested.
- Coordinate interfaces and source ownership before parallel changes to shared boot, linker, kernel-header, or build files.
- Never create unattended workflows that push commits, publish releases, access secrets, or run unreviewed code. CI should build, test, and upload artifacts only.
