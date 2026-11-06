# Development-only build environment for MyOS.
# This is not a runtime/container image for the operating system.
FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential gcc binutils make nasm \
      grub-common grub-pc-bin xorriso mtools \
      qemu-system-x86 gdb python3 file git ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
CMD ["bash"]
