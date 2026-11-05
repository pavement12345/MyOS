/* Multiboot2 x86 entry: switch GRUB's 32-bit protected mode into long mode. */
BITS 32

section .multiboot
align 8
mb2_header_start:
    dd 0xE85250D6
    dd 0
    dd mb2_header_end - mb2_header_start
    dd -(0xE85250D6 + (mb2_header_end - mb2_header_start))
    dw 0
    dw 0
    dd 8
mb2_header_end:

section .text
align 16
global _start
extern kernel_main

_start:
    cli
    cmp eax, 0x36D76289
    jne .halt32
    mov [boot_magic], eax
    mov [boot_info], ebx

    ; Temporary identity map for the first 1 GiB using 2 MiB pages.
    mov eax, pdpt_table
    or eax, 0x03
    mov [pml4_table], eax
    mov eax, page_directory
    or eax, 0x03
    mov [pdpt_table], eax
    mov dword [pdpt_table + 4], 0
    xor ecx, ecx
.map:
    mov eax, ecx
    shl eax, 21
    or eax, 0x83
    mov [page_directory + ecx * 8], eax
    mov dword [page_directory + ecx * 8 + 4], 0
    inc ecx
    cmp ecx, 512
    jne .map

    lgdt [gdt_pointer]
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    mov eax, pml4_table
    mov cr3, eax
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    jmp 0x18:long_mode_start

.halt32:
    hlt
    jmp .halt32

BITS 64
long_mode_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    xor eax, eax
    mov fs, ax
    mov gs, ax
    lea rsp, [rel stack_top]
    and rsp, -16
    mov edi, [rel boot_magic]
    mov esi, [rel boot_info]
    call kernel_main
.halt64:
    cli
    hlt
    jmp .halt64

section .rodata
align 8
gdt:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
    dq 0x00AF9A000000FFFF
gdt_end:
gdt_pointer:
    dw gdt_end - gdt - 1
    dd gdt

section .bss
align 4096
pml4_table: resq 512
pdpt_table: resq 512
page_directory: resq 512
align 16
stack_bottom: resb 16384
stack_top:
boot_magic: resd 1
boot_info: resd 1

section .note.GNU-stack noalloc noexec nowrite progbits
