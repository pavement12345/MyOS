#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
KERNEL="$ROOT/build/kernel.elf"
ISO="$ROOT/build/MyOS.iso"
ISO_ROOT="$ROOT/build/iso-root"

[[ -s "$KERNEL" ]] || { echo "Kernel missing or empty; run 'make kernel' first." >&2; exit 1; }
command -v grub-mkrescue >/dev/null || { echo "grub-mkrescue not found; run 'make setup'." >&2; exit 1; }
command -v xorriso >/dev/null || { echo "xorriso not found; run 'make setup'." >&2; exit 1; }
[[ -f "$ROOT/boot/grub/grub.cfg" ]] || { echo "GRUB configuration missing." >&2; exit 1; }

bash "$ROOT/tools/check_kernel.sh" "$KERNEL"
rm -rf "$ISO_ROOT"
mkdir -p "$ISO_ROOT/boot/grub" "$ROOT/build"
install -m 0644 "$KERNEL" "$ISO_ROOT/boot/kernel.elf"
install -m 0644 "$ROOT/boot/grub/grub.cfg" "$ISO_ROOT/boot/grub/grub.cfg"
rm -f "$ISO"
grub-mkrescue -o "$ISO" "$ISO_ROOT"
[[ -s "$ISO" ]] || { echo "ISO generation failed: output missing or empty." >&2; exit 1; }
printf 'Created GRUB BIOS ISO: %s (%s bytes)\n' "$ISO" "$(stat -c '%s' "$ISO")"
