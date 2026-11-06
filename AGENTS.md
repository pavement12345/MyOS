# MyOS — Agent & Contributor Engineering Instructions

## 1. Project Identity, Goals & Engineering Principles

MyOS is an independently developed x86_64 operating system. Linux is strictly a development and build environment, not the operating system's foundation or runtime.

All core operating system functionality must be implemented as part of MyOS rather than delegated to an existing Linux installation or distribution.

Follow these principles:

- **Incremental development:** Implement one coherent subsystem or feature at a time.
- **Architectural integrity:** Keep the kernel, hardware abstraction, drivers, system services, and desktop components properly separated.
- **Evidence-based development:** Never claim functionality works without corresponding implementation and test evidence.
- **No fake functionality:** Never introduce placeholders that return success, simulate hardware, fabricate output, or disguise unfinished functionality.
- **Explicit limitations:** Clearly document incomplete subsystems, unsupported hardware, architectural dependencies, and known issues.
- **Maintainability:** Prefer understandable, modular implementations over unnecessarily complicated abstractions.
- **Resource awareness:** Keep performance, memory consumption, boot time, and binary size in mind.

Never sacrifice correctness or architectural stability simply to make a feature appear complete.

## 2. Repository Structure & File Ownership

Preserve the existing repository structure and follow the project brief.

Use the established directories:

- `boot/` — Bootloader integration, boot protocol handling, architecture-specific startup code.
- `kernel/` — Kernel initialization, core services, interrupt handling, system interfaces.
- `memory/` — Physical and virtual memory management, paging, allocation.
- `drivers/` — Hardware discovery, device drivers, and hardware abstraction.
- `filesystem/` — Filesystem implementations, VFS, storage interfaces.
- `tools/` — Development utilities, compilers, generators, and supporting tools.
- `scripts/` — Build, testing, automation, and maintenance scripts.
- `docs/` — Architecture documentation, subsystem specifications, and development notes.

Additional directories may be introduced when justified by architectural requirements.

Rules:

- Do not rename, relocate, or restructure established directories without a concrete reason.
- Keep `linker.ld` at the repository root unless a documented architectural requirement justifies changing it.
- Keep generated objects, ELF files, disk images, ISOs, temporary files, and build caches under `build/`.
- Never commit generated binaries or temporary build artifacts.
- Avoid unnecessary files, duplicated implementations, and excessive abstraction layers.
- Every new subsystem should have a clearly defined location and responsibility.

## 3. Kernel Architecture & Implementation

### Core requirements

- Kernel implementation must use freestanding C and assembly.
- Do not link host libc, host runtime libraries, or Linux-specific kernel interfaces into MyOS.
- Compile architecture-specific C and NASM sources using appropriate independent toolchain rules.
- Preserve dependency generation and incremental compilation.
- Use explicit types and well-defined interfaces for hardware and kernel structures.
- Avoid undefined behavior, unsafe pointer arithmetic, unvalidated memory access, and implicit assumptions about hardware state.
- Keep architecture-dependent code isolated from architecture-independent functionality.

### Boot process

- Respect the boot protocol's register, memory, and calling-convention requirements.
- Validate all bootloader-provided structures before accessing them.
- Validate memory maps, framebuffer information, module addresses, and other boot-provided resources.
- Establish the required CPU execution environment before initializing dependent subsystems.
- Do not enable interrupts until the IDT, exception handlers, and required interrupt-controller configuration are valid.
- Preserve reliable failure reporting during early initialization.

### Kernel subsystem dependencies

Do not implement higher-level functionality before its required foundations exist.

Examples:

| Subsystem | Required foundations |
|---|---|
| Heap allocator | Physical memory management and virtual memory |
| Process management | Memory isolation and execution context management |
| User accounts | Storage, filesystem, permissions, and process isolation |
| Filesystem | Storage interfaces and memory management |
| Networking | Device drivers, packet buffers, and network stack |
| Browser | Networking, application interface, and rendering capabilities |
| Desktop environment | Graphics subsystem, input handling, and application interface |
| Audio | Hardware support and audio interfaces |

These are architectural dependencies, not permission to create simulated implementations.

## 4. Hardware Discovery & Driver Development

Hardware discovery and functional device support are separate engineering milestones.

For PCI development:

- Validate configuration-space access.
- Bound device enumeration to valid bus, device, and function ranges.
- Identify devices using vendor and device IDs.
- Validate multifunction-device behavior.
- Handle missing devices and inaccessible configuration space safely.
- Document discovered hardware without claiming functional driver support.

For device drivers:

- Implement actual hardware initialization and communication.
- Validate device state before issuing commands.
- Handle interrupts, DMA, memory-mapped I/O, and port I/O according to device specifications.
- Respect hardware ownership, alignment, and memory-mapping requirements.
- Handle initialization failures and unsupported devices explicitly.
- Keep individual drivers independent of unrelated kernel subsystems.

A detected PCI device is not necessarily an initialized device.

An initialized device is not necessarily a functional driver.

A functional driver does not automatically provide a complete operating system service.

For example, discovering an Intel e1000 NIC does not establish network connectivity. Actual driver initialization, packet transmission, reception, and network-stack integration must be implemented and tested independently.

## 5. Desktop & Application Architecture

Desktop functionality must remain separate from the freestanding kernel.

- C++ desktop components must not introduce host runtime dependencies into the kernel.
- Desktop applications must use an explicitly defined application interface.
- Do not assume desktop components execute directly inside the kernel.
- Keep rendering, window management, input handling, and application lifecycle responsibilities separated.
- Define stable interfaces between the kernel and higher-level components.
- Do not claim graphical functionality based solely on QEMU displaying a framebuffer or reaching a boot screen.

The desktop architecture should support independently developed components without requiring modifications to unrelated kernel subsystems.

## 6. Security & Reliability

Security must be considered throughout development rather than added after functionality is implemented.

- Validate external input, hardware responses, and boot-provided data.
- Enforce memory boundaries and avoid uncontrolled memory access.
- Do not assume hardware devices behave correctly.
- Handle invalid pointers, malformed structures, and unavailable resources safely.
- Avoid exposing kernel memory to untrusted applications.
- Keep privileged operations separate from future user-mode functionality.
- Never introduce arbitrary code execution, unrestricted memory access, or unnecessary privilege escalation.
- Do not store credentials or secrets in source files, scripts, documentation, or CI configuration.
- Never execute downloaded scripts or binaries without reviewing their origin and integrity.

Incomplete security mechanisms must be documented honestly.

## 7. Development Workflow & Change Management

Before making changes:

1. Inspect the existing implementation, repository structure, documentation, and build configuration.
2. Identify the subsystem being modified and its dependencies.
3. Check whether an existing implementation or interface already exists.
4. Identify potential conflicts with other contributors or agents.
5. Establish a clear implementation and verification plan.

During development:

1. Make the smallest coherent change that advances the current milestone.
2. Preserve existing functionality unless modification is necessary.
3. Follow established naming conventions and coding styles.
4. Keep interfaces consistent across subsystem boundaries.
5. Add meaningful error handling and validation.
6. Avoid unrelated refactoring.
7. Document architectural decisions when they affect future development.

After development:

1. Inspect all modified files.
2. Review the complete Git diff.
3. Check for accidental deletions, generated files, and unrelated modifications.
4. Verify that implementation claims match actual behavior.
5. Update documentation where necessary.
6. Report verification results and outstanding limitations.

## 8. Build System & Verification

The build system must remain reproducible and independent of the host operating system's runtime.

For relevant build changes, run:

```bash
make clean
make kernel
make check
make iso
```

For boot-related changes:

- Boot the generated ISO using QEMU.
- Verify that the kernel reaches the expected initialization stage.
- Inspect serial output and relevant diagnostic information.
- Verify that newly implemented functionality behaves as expected.
- Check for exceptions, hangs, invalid memory access, and unexpected resets.

For hardware-related changes:

- Use appropriate QEMU device emulation when available.
- Verify actual device initialization rather than discovery alone.
- Test error conditions and missing-device scenarios.
- Clearly distinguish emulated hardware validation from physical hardware validation.

For every test, report:

- Exact command executed.
- Whether it passed, failed, or could not be executed.
- Relevant output or evidence.
- Any known limitations.

Never report a test as passing because it was expected to pass.

If dependencies are unavailable, explain the limitation and provide the exact command needed to perform the missing verification.

## 9. Documentation & Project Roadmap

Keep documentation synchronized with actual implementation.

Update `README.md` when externally visible functionality, setup requirements, or project capabilities change.

Update `docs/roadmap.md` when milestones, subsystem status, dependencies, or implementation priorities change.

Maintain architecture documentation describing:

- Subsystem responsibilities.
- Internal interfaces and dependencies.
- Initialization order.
- Hardware requirements.
- Memory ownership.
- Known limitations.
- Future integration requirements.

Use explicit subsystem statuses:

- `NOT STARTED` — No implementation exists.
- `IN PROGRESS` — Implementation is incomplete.
- `IMPLEMENTED` — Implementation exists but has not passed sufficient verification.
- `TESTED` — Relevant automated or manual tests have passed.
- `INTEGRATED` — Integrated with dependent subsystems and verified.
- `BLOCKED` — Development cannot proceed because of an identified dependency or issue.

Do not promote a subsystem to a higher status without evidence.

## 10. Multi-Agent Development & Contributor Coordination

Agents may independently implement, investigate, or review bounded subsystems.

However:

- Each agent must inspect the current repository state before working.
- Never assume another agent's changes have been merged, tested, or integrated.
- Define ownership boundaries before parallel work.
- Coordinate changes to shared headers, boot code, linker configuration, kernel initialization, and build files.
- Avoid multiple agents independently implementing the same subsystem.
- Do not overwrite another contributor's work without reviewing it.
- Communicate interface changes to affected contributors.
- Integrate and test changes before marking shared milestones complete.

Agents should provide clear implementation reports containing:

1. What was changed.
2. Which files were modified or added.
3. What functionality was actually implemented.
4. What remains unfinished.
5. Which tests were executed.
6. Their exact results.
7. Any integration requirements or architectural concerns.

Agents must not create unattended workflows that push commits, publish releases, access secrets, or execute unreviewed code.

CI may build, test, and upload generated artifacts, but must not independently publish releases or modify the repository without explicit authorization.

## 11. Definition of Done

A feature is not complete merely because its source code compiles.

Before considering a feature complete:

- Its intended behavior must be implemented.
- Its dependencies must exist.
- Its interfaces must be integrated.
- Relevant error conditions must be handled.
- Relevant automated or manual tests must pass.
- Documentation must reflect its actual capabilities.
- The Git diff must be reviewed.
- Remaining limitations must be disclosed.

**Final engineering rule:** Build a real operating system, not a collection of demonstrations that only appear functional. Every subsystem must have a defined responsibility, a real implementation, verified behavior, and a documented integration path.
