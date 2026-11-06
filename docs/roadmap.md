# MyOS development roadmap

This roadmap describes the current `copilot/myos-development` branch. “Implemented in source” means code exists; it does **not** mean it is integrated, built, or runtime-verified. Only mark an item verified after recording successful build/test evidence.

## Current branch status

- **Boot baseline:** GRUB Multiboot2 BIOS entry, NASM transition to long mode, freestanding kernel entry, serial/VGA boot messages, panic path. Existing baseline; verify against current branch with CI.
- **Serial shell pieces (not integrated):** COM1 polled UART driver, bounded serial shell input loop, basic command handlers (`help`, `clear`, `echo`, `version`, `systeminfo`, `halt`). The current `kernel/main.c` still runs the original halt path, and the Makefile does not compile these shell/driver files. Do not treat the shell as usable until integrated and tested.
- **Network device API/registry (not integrated, no NIC support):** `drivers/net/netdev.h` and `netdev.c` define static registration and callbacks for up to eight Ethernet devices. No PCI discovery, NIC drivers, protocol stack, or network management commands exist. The Makefile currently does not compile the registry.
- **Verification:** No successful current-branch build, ISO generation, or QEMU boot result is recorded in this roadmap.

## Near-term milestones

### 1. Restore and verify the boot baseline

- Run `make clean`, `make kernel`, `make check`, and `make iso` on the development branch.
- Fix all build failures and preserve the BIOS Multiboot2 validation.
- Run a bounded QEMU serial boot smoke test and confirm expected boot messages.
- Record CI results; do not infer passing status from source alone.

### 2. Integrate and verify the serial shell

- Add the serial driver and shell/command sources to Makefile compilation and include paths.
- Initialize COM1 and launch `shell_run()` from the validated kernel entry.
- Keep interrupts disabled and shell input polled until a valid interrupt/exception path exists.
- Extend CI to confirm the shell-ready message and prompt; test commands interactively in QEMU (`help`, `echo`, `clear`, unknown command, backspace, maximum line length, `halt`).
- Document that this is a kernel-context COM1 shell, not a user process or keyboard/GUI shell.

### 3. Establish safe hardware foundations

- Parse and validate Multiboot2 information, including memory-map bounds.
- Add CPU exception handling and an IDT before enabling interrupts.
- Implement PCI configuration-space discovery and a bounded device inventory.
- Design safe MMIO mapping and DMA buffer allocation/lifetime rules before writing bus-mastering drivers.
- Add tests or QEMU diagnostics for invalid device data and unsupported hardware.

### 4. Build networking incrementally

- Wire `netdev.c` into the build and add tests for registration limits, invalid callbacks/MTU, start/stop, and frame-length bounds.
- Choose an initial QEMU NIC model and implement one driver only after PCI, MMIO/DMA, and memory-management prerequisites are met. Document the exact QEMU device configuration.
- Validate link state and frame transmit/receive with a QEMU-only test harness; do not claim physical NIC compatibility.
- Add Ethernet framing and then a small, testable network layer (ARP and IPv4/ICMP before UDP/TCP or DHCP). Define packet-buffer ownership and bounds checks.
- Add a text adapter interface inspired by Windows Network Connections: list adapters, inspect link/MAC/MTU/driver, and enable/disable an adapter. Keep it explicitly informational/control-only; no GUI exists yet.
- Add additional NIC drivers one at a time behind the common `netdev_ops` interface, each with documented supported IDs, limitations, and tests.

## Later milestones

1. Physical/virtual memory managers and a kernel heap after memory-map parsing.
2. Timer and scheduler after exception/interrupt/controller support.
3. Block storage, filesystem, and VFS after stable device and memory APIs.
4. User mode and system calls after paging, exceptions, and process foundations.
5. Keyboard input and reusable graphics; only then consider a graphical desktop/settings UI.
6. Broader hardware support, security review, regression tests, performance work, and release process.
7. UEFI boot as a separately tested boot path; BIOS-only support remains the current target.

## Known limitations

The bootstrap identity-maps only the first GiB and uses a minimal GDT. Multiboot2 data is not yet parsed. There is no verified IDT/interrupt framework, memory manager, PCI subsystem, network driver, network stack, filesystem, process model, syscall interface, keyboard driver, or graphical UI. No physical-hardware support or release-ready ISO is claimed.
