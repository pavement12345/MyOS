#!/usr/bin/env bash
set -euo pipefail

for tool in git make gcc ld nasm grub-file grub-mkrescue xorriso qemu-system-x86_64 gdb python3; do
  command -v "$tool" >/dev/null || { echo "Missing required tool: $tool (run make setup)" >&2; exit 1; }
done
case "$(gcc -dumpmachine)" in
  x86_64-*) ;;
  *) echo "GCC must target x86_64; got $(gcc -dumpmachine)" >&2; exit 1 ;;
esac

echo "Host tool check passed. GCC target: $(gcc -dumpmachine)"
