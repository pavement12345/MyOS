# MyOS development roadmap

This roadmap separates code present in source from behavior integrated and tested in the running kernel.

## Current milestone

- **Boot/UI/memory/filesystem foundations:** see [architecture](architecture.md) and [kernel notes](kernel.md); build and runtime status must be established by CI or local tests.
- **Implemented networking foundations:** bounded PCI discovery and QEMU e1000 controller reset/MAC/link inspection exist. The network-device registry is now included in the kernel build and has host-side mock-driver tests.
- **Not implemented:** e1000 DMA rings or packet I/O, network protocol stack (Ethernet/ARP/IP/TCP/UDP/DNS/DHCP), Internet access, firewall/policy services, user-mode networking tools, or physical NIC support.

## Networking milestones

1. Run `make test-network`; test registration validation/capacity, device lifecycle, transmit bounds, and receive callback length checks.
2. Implement an e1000 packet backend only after defining DMA allocation, cache coherency, descriptor ownership, and teardown. Validate in an isolated QEMU user-mode network with a private test service.
3. Implement bounded protocol modules and parser tests for malformed/truncated frames; proceed from Ethernet/ARP and IPv4/ICMP to UDP/TCP, then DHCP/DNS.
4. Add a deny-by-default interface policy and auditable, explicit configuration. Limit active exercises to authorized labs and require user-selected targets.
5. Do not claim general Internet access until end-to-end DNS and transport tests pass against controlled services. Do not claim physical hardware support without hardware-specific validation.

## Existing limitations

Maskable interrupts remain disabled. The kernel has no process isolation, user mode, persistent storage, memory protection, or tested DMA API. The e1000 initialization code does not transmit or receive frames. This project is not a production-ready defensive appliance and does not provide active scanning or exploit automation.
