#!/usr/bin/env bash
set -euo pipefail

if [[ "$(id -u)" -eq 0 ]]; then
  SUDO=()
elif command -v sudo >/dev/null 2>&1; then
  SUDO=(sudo)
else
  echo "Error: setup needs root or sudo to install packages." >&2
  exit 1
fi

export DEBIAN_FRONTEND=noninteractive
"${SUDO[@]}" apt-get update
"${SUDO[@]}" apt-get install -y --no-install-recommends \
  build-essential gcc binutils make nasm grub-common grub-pc-bin \
  xorriso qemu-system-x86 gdb git python3 file mtools ca-certificates

for tool in gcc ld nasm grub-file grub-mkrescue xorriso qemu-system-x86_64 gdb python3; do
  command -v "$tool" >/dev/null || { echo "Missing tool after installation: $tool" >&2; exit 1; }
done

target="$(gcc -dumpmachine)"
case "$target" in
  x86_64-*) echo "GCC target validated: $target" ;;
  *) echo "Expected an x86_64 GCC target, found: $target" >&2; exit 1 ;;
esac

echo "MyOS development dependencies installed."
echo "Kernel compilation uses freestanding flags and does not link host libc."
