# MyOS

MyOS is an experimental independent x86_64 operating-system project, built as freestanding C and NASM and booted through GRUB Multiboot2 BIOS. It currently provides an early kernel, a QEMU-oriented framebuffer desktop, low-level memory/filesystem foundations, bounded PCI discovery, and e1000 controller initialization. It is not a general-purpose OS.

## Networking status

The generic network-device registry is compiled into the kernel and covered by mock-driver unit tests. PCI discovery can identify the QEMU e1000, and the current driver initialization reads controller MAC/link state. **There is not yet a functional NIC packet path, TCP/IP stack, DNS/DHCP, or Internet connectivity.** The host tests validate the registry contract only; they do not prove network hardware support.

## Build and test

Ubuntu/Debian x86_64 or GitHub Codespaces:

```sh
make setup
make clean
make kernel
make check
make iso
make test-network
```

Run the ISO in QEMU using `make run`. The existing Actions workflow performs a bounded QEMU boot smoke test. Host networking tests use mock callbacks and require only a native GCC compiler; they do not send packets to a network.

## Network development direction

The intended path is to establish safe DMA ownership and packet I/O in an isolated QEMU network, then add tested Ethernet/ARP/IP/transport/name-resolution layers and explicit network policy. Detailed dependencies and acceptance gates are in [the roadmap](docs/roadmap.md). Network experiments should be limited to systems and lab targets the operator is authorized to test.

## Other project documentation

- [Architecture](docs/architecture.md)
- [Kernel notes](docs/kernel.md)
- [Contributing](CONTRIBUTING.md)
- [Security policy](SECURITY.md)

Windows PE `.exe` support, persistent filesystems, user mode, a full network stack, and physical-hardware compatibility are not implemented.
