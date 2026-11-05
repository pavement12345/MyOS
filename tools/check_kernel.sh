#!/usr/bin/env bash
set -euo pipefail

kernel="${1:-build/kernel.elf}"
[[ -f "$kernel" ]] || { echo "Kernel not found: $kernel" >&2; exit 1; }
for tool in grub-file readelf file; do
  command -v "$tool" >/dev/null || { echo "Required validation tool missing: $tool" >&2; exit 1; }
done
file "$kernel"
readelf -h "$kernel" | grep -Eq 'Class:[[:space:]]+ELF64' || { echo "Kernel is not ELF64" >&2; exit 1; }
grub-file --is-x86-multiboot2 "$kernel" || { echo "Kernel has no valid GRUB Multiboot2 header" >&2; exit 1; }
echo "Kernel validation passed: ELF64 and Multiboot2 header found."
