# MyOS development roadmap

This roadmap distinguishes code in the repository from integrated and verified behavior. Do not claim a capability until its implementation and relevant tests have been exercised.

## Current state

- **Kernel/UI/memory/VFS:** early boot and framebuffer desktop with polled input, exception-only IDT, memory foundations, and volatile RAM filesystem. See [architecture](architecture.md) and [kernel notes](kernel.md); builds still need verification on the branch.
- **Networking foundations:** registry API with static-lifetime device contract, separate started/carrier state, bounds checks on frames and callback-reported receive lengths; host mock-driver tests are available. Pure parser handles Ethernet, ARP Ethernet/IPv4, and IPv4 headers with truncation/malformed checks.
- **e1000 status:** PCI discovery and controller reset/MAC/link inspection only. No DMA rings, frame transmit/receive, or physical NIC support.
- **Not implemented:** live packet path, IPv4/ARP stack integration, TCP/UDP, DHCP/DNS, Internet connectivity, firewall/policy services, or user-facing network tools.

## Networking steps

1. Verify `make test-network` and normal kernel build in CI.
2. Specify and test DMA allocation, mapping/cache behavior, descriptor ownership, and teardown before enabling e1000 bus mastering.
3. Add QEMU-only e1000 TX/RX tests using an isolated user-mode network and a private local test service.
4. Integrate bounded Ethernet/ARP/IPv4 parsing with explicit packet-buffer ownership; expand synthetic tests for options, fragments, invalid lengths, and protocol boundaries.
5. Add transport and name-resolution components only after prior layers are tested. Validate DHCP/DNS against controlled QEMU services before claiming Internet access.
6. Add deny-by-default configuration, explicit user review, and auditable network status. Restrict active lab exercises to authorized targets.

## Constraints

Maskable interrupts remain disabled. There is no tested DMA API, process isolation, user mode, persistent storage, memory protection, or production-ready security boundary. Host-side tests only exercise pure functions and mocks; they do not prove kernel runtime or hardware behavior.
