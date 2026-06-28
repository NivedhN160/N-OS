MULTIBOOT_PAGE_ALIGN   equ 1<<0
MULTIBOOT_MEMORY_INFO  equ 1<<1
MULTIBOOT_VIDEO_MODE   equ 1<<2
MULTIBOOT_HEADER_MAGIC equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE
CHECKSUM               equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

section .multiboot
align 4
    dd MULTIBOOT_HEADER_MAGIC
    dd MULTIBOOT_HEADER_FLAGS
    dd CHECKSUM
    dd 0, 0, 0, 0, 0 ; header_addr, load_addr, load_end_addr, bss_end_addr, entry_addr
    dd 0             ; mode_type (0 = linear graphics)
    dd 1024          ; width
    dd 768           ; height
    dd 32            ; depth

section .text
global _start
extern kernel_main

_start:
    cli
    mov esp, stack_top
    push ebx ; push multiboot info pointer
    push eax ; push magic number
    call kernel_main
    hlt

section .bss
resb 32768
stack_top:
