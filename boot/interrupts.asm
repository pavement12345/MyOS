BITS 64

section .text

extern interrupt_exception_handler
global isr_unhandled
global exception_page_fault_test
global page_fault_test_load
global page_fault_test_recovery

%macro ISR_NO_ERROR 1
global isr_%1
isr_%1:
    push qword 0
    push qword %1
    jmp isr_common
%endmacro

%macro ISR_ERROR 1
global isr_%1
isr_%1:
    push qword %1
    jmp isr_common
%endmacro

ISR_NO_ERROR 0
ISR_NO_ERROR 1
ISR_NO_ERROR 2
ISR_NO_ERROR 3
ISR_NO_ERROR 4
ISR_NO_ERROR 5
ISR_NO_ERROR 6
ISR_NO_ERROR 7
ISR_ERROR 8
ISR_NO_ERROR 9
ISR_ERROR 10
ISR_ERROR 11
ISR_ERROR 12
ISR_ERROR 13
ISR_ERROR 14
ISR_NO_ERROR 15
ISR_NO_ERROR 16
ISR_ERROR 17
ISR_NO_ERROR 18
ISR_NO_ERROR 19
ISR_NO_ERROR 20
ISR_ERROR 21
ISR_NO_ERROR 22
ISR_NO_ERROR 23
ISR_NO_ERROR 24
ISR_NO_ERROR 25
ISR_NO_ERROR 26
ISR_NO_ERROR 27
ISR_NO_ERROR 28
ISR_ERROR 29
ISR_ERROR 30
ISR_NO_ERROR 31

exception_page_fault_test:
    mov rax, 0x100000000
page_fault_test_load:
    mov rax, [rax]
page_fault_test_recovery:
    ret

isr_unhandled:
    push qword 0
    push qword 255
    jmp isr_common

isr_common:
    cld
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov r12, rsp
    mov rdi, [r12 + 120]
    mov rsi, [r12 + 128]
    lea rdx, [r12 + 136]
    and rsp, -16
    call interrupt_exception_handler
    mov rsp, r12

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 16
    iretq

section .rodata
align 8
global isr_stub_table
isr_stub_table:
%macro ISR_TABLE_ENTRY 1
    dq isr_%1
%endmacro
ISR_TABLE_ENTRY 0
ISR_TABLE_ENTRY 1
ISR_TABLE_ENTRY 2
ISR_TABLE_ENTRY 3
ISR_TABLE_ENTRY 4
ISR_TABLE_ENTRY 5
ISR_TABLE_ENTRY 6
ISR_TABLE_ENTRY 7
ISR_TABLE_ENTRY 8
ISR_TABLE_ENTRY 9
ISR_TABLE_ENTRY 10
ISR_TABLE_ENTRY 11
ISR_TABLE_ENTRY 12
ISR_TABLE_ENTRY 13
ISR_TABLE_ENTRY 14
ISR_TABLE_ENTRY 15
ISR_TABLE_ENTRY 16
ISR_TABLE_ENTRY 17
ISR_TABLE_ENTRY 18
ISR_TABLE_ENTRY 19
ISR_TABLE_ENTRY 20
ISR_TABLE_ENTRY 21
ISR_TABLE_ENTRY 22
ISR_TABLE_ENTRY 23
ISR_TABLE_ENTRY 24
ISR_TABLE_ENTRY 25
ISR_TABLE_ENTRY 26
ISR_TABLE_ENTRY 27
ISR_TABLE_ENTRY 28
ISR_TABLE_ENTRY 29
ISR_TABLE_ENTRY 30
ISR_TABLE_ENTRY 31

section .note.GNU-stack noalloc noexec nowrite progbits
