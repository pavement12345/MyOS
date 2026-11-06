.DEFAULT_GOAL := help
SHELL := /bin/bash

BUILD := build
OBJ := $(BUILD)/obj
KERNEL := $(BUILD)/kernel.elf
ISO := $(BUILD)/MyOS.iso
# Use the validated host GCC unless explicitly overridden on the make command line.
CC = gcc
NASM = nasm
QEMU = qemu-system-x86_64

CFLAGS := -std=gnu11 -O2 -Wall -Wextra -Werror -ffreestanding \
          -fno-stack-protector -fno-pic -fno-pie -m64 -mno-red-zone \
          -mno-mmx -mno-sse -mno-sse2 -mcmodel=small -Ikernel -Idrivers/input
LDFLAGS := -m64 -nostdlib -static -no-pie -Wl,-T,linker.ld \
           -Wl,--build-id=none -Wl,-z,max-page-size=0x1000

C_SOURCES := kernel/main.c kernel/panic.c kernel/framebuffer.c kernel/desktop.c \
             drivers/input/keyboard.c drivers/input/mouse.c
ASM_SOURCES := boot/boot.asm
C_OBJECTS := $(patsubst %.c,$(OBJ)/%.o,$(C_SOURCES))
ASM_OBJECTS := $(patsubst %.asm,$(OBJ)/%.o,$(ASM_SOURCES))
OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

.PHONY: all help setup clean check kernel iso run debug

all: kernel

help:
	@echo "MyOS build targets: setup clean check kernel iso run debug"

setup:
	@bash scripts/setup.sh

$(OBJ)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJ)/%.o: %.asm
	@mkdir -p $(@D)
	$(NASM) -f elf64 $< -o $@

$(KERNEL): $(OBJECTS) linker.ld
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

kernel: $(KERNEL)
	@file $(KERNEL)

iso: $(ISO)

$(ISO): $(KERNEL) boot/grub/grub.cfg tools/create_iso.sh tools/check_kernel.sh
	@bash tools/check_kernel.sh $(KERNEL)
	@bash tools/create_iso.sh

check: $(KERNEL)
	@bash tools/check_kernel.sh $(KERNEL)

run: $(ISO)
	$(QEMU) -cdrom $(ISO) -m 256M -serial stdio -no-reboot -no-shutdown

debug: $(ISO)
	@echo "QEMU GDB stub: localhost:1234; attach with: gdb $(KERNEL) -ex 'target remote :1234'"
	$(QEMU) -cdrom $(ISO) -m 256M -serial stdio -no-reboot -no-shutdown -S -s

clean:
	rm -rf $(OBJ) $(BUILD)/iso-root $(KERNEL) $(ISO) $(BUILD)/qemu.log

-include $(C_OBJECTS:.o=.d)
