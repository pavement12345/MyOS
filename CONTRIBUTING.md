# Contributing to MyOS

Thanks for helping build an independent x86_64 operating system. Contributions should be technically accurate, reviewable, and explicit about what is implemented versus tested.

## Before you start

1. Read [AGENTS.md](AGENTS.md), [architecture](docs/architecture.md), [kernel notes](docs/kernel.md), and the [roadmap](docs/roadmap.md).
2. Check existing issues and pull requests so work does not conflict or duplicate effort.
3. For architecture, boot, linker, kernel-header, or build-system changes, discuss interfaces and ownership before implementing a broad change.
4. Keep pull requests focused. Avoid unrelated formatting churn and generated binaries.

## Development environment

Use Ubuntu/Debian x86_64 or Codespaces. Install dependencies with `make setup`, then validate with `bash scripts/check.sh`. The required tools and commands are documented in the README.

## Engineering requirements

- Kernel implementation is freestanding C/assembly; do not link host libc or runtime libraries into the kernel.
- Keep architecture-specific startup in `boot/`, kernel subsystems in `kernel/`, memory code in `memory/`, drivers in `drivers/`, filesystem code in `filesystem/`, shell code in `shell/`, and build utilities in `tools/` or `scripts/`.
- Preserve the root linker script name `linker.ld` unless a documented technical reason requires a change.
- Validate boot-provided structures and bounds before dereferencing or using them.
- Do not enable interrupts before valid IDT, exception, and interrupt-controller setup.
- Do not claim memory allocation, filesystem access, networking, keyboard support, user mode, executable loading, or graphics before their prerequisites exist and tests exercise them.
- Avoid fake stubs that report success without performing the operation. Return meaningful errors and document unsupported cases.
- Keep objects, ELF files, disk images, ISOs, and other generated output under `build/`; never commit them.
- Do not add secrets, telemetry, publish/release automation, or workflows that push code.

## Checks

For build-system or kernel changes, run:

```sh
make clean
make kernel
make check
make iso
```

For boot changes, also run the ISO in QEMU and verify the serial output. For driver changes, include a test plan that names the device model, QEMU configuration, expected behavior, and unsupported cases. CI currently checks the kernel/ISO and boot messages; it does not prove interactive shell or network functionality.

In the pull request description, include:

- Purpose and scope.
- Design/interface decisions and alternatives where relevant.
- Commands run and their actual results. Clearly list checks not run and why.
- QEMU configuration and observed serial output for boot/device changes.
- Known limitations, follow-up work, and documentation updates.

## Pull request lifecycle

1. Open a focused pull request from a topic branch; do not target `main` with unrelated work.
2. Keep commits understandable and respond to review comments with evidence.
3. Ensure CI passes, documentation matches behavior, and generated artifacts are absent.
4. Request review from maintainers. Do not merge your own change unless repository policy explicitly allows it.

## Reporting bugs and requesting features

Use the GitHub issue forms. Bug reports should include host/tool versions, exact commands, logs, and reproducible QEMU configuration. Feature requests should describe the user need, prerequisites, proposed scope, and how success will be tested. Never include secrets or sensitive data in logs.
