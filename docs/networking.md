# MyOS network foundations

`netdev.*` defines a bounded, static-lifetime device registry and callback interface. A driver reports physical/virtual carrier through `link_up`; the core tracks administrative startup separately with `started`. Frame input is bounded by the device MTU plus Ethernet header/FCS allowances. `netdev_receive` clears the output length before calling a driver and validates its reported frame length before returning it.

`net_packet.*` contains allocation-free parser functions for borrowed Ethernet frame buffers:

- Ethernet II header and EtherType.
- ARP with Ethernet hardware and IPv4 protocol address formats.
- IPv4 base/options header lengths, total length, protocol, addresses, and fragmentation flags/offset.

The parser does not verify IPv4 checksums, reassemble fragments, interpret transport payloads, capture traffic, open sockets, or transmit packets. Callers must keep the original frame buffer alive while using returned views and must not treat unparsed payloads as trusted.

Run the host tests:

```sh
make test-network
```

Tests use synthetic data only. They are not an e1000 driver test. Current e1000 initialization only reads controller state; DMA and packet I/O are still unimplemented.
