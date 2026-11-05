.DEFAULT_GOAL := help
SHELL := /bin/bash

.PHONY: help setup clean check kernel iso run debug

help:
	@echo "MyOS development targets:"
	@echo "  make setup  Install host development dependencies"
	@echo "  make check  Verify Phase 1 host tools"
	@echo "  make clean  Remove generated build output"
	@echo "  make kernel Build the kernel (available in a later phase)"
	@echo "  make iso    Generate build/MyOS.iso (available in a later phase)"
	@echo "  make run    Run MyOS in QEMU (available in a later phase)"
	@echo "  make debug  Start a QEMU/GDB debugging session (available later)"

setup:
	@bash scripts/setup.sh

check:
	@set -euo pipefail; \
	missing=0; \
	for tool in git make gcc ld nasm python3 grub-mkrescue xorriso qemu-system-x86_64 gdb; do \
		if ! command -v "$$tool" >/dev/null 2>&1; then \
			echo "Missing required tool: $$tool"; \
			missing=1; \
		else \
			printf "Found %-24s %s\n" "$$tool" "$$(command -v "$$tool")"; \
		fi; \
	done; \
	if [[ "$$missing" -ne 0 ]]; then \
		echo "Run 'make setup' and then 'make check' again."; \
		exit 1; \
	fi; \
	echo "Phase 1 host-tool check passed."; \
	echo "The kernel cross-compiler and bootable ISO are not implemented yet."

clean:
	@mkdir -p build
	@rm -rf build/obj build/iso-root build/MyOS.iso build/kernel.elf
	@echo "Removed generated MyOS build output."

kernel:
	@echo "Not implemented yet: kernel build is scheduled for Phase 2 and later." >&2
	@exit 2

iso:
	@echo "Not implemented yet: bootable ISO generation follows the kernel build." >&2
	@exit 2

run:
	@echo "Not implemented yet: QEMU boot testing follows ISO generation." >&2
	@exit 2

debug:
	@echo "Not implemented yet: QEMU/GDB debugging follows the kernel build." >&2
	@exit 2
