; TradeKernel - Multiboot2 Compatibility Header
; Designed for direct loading with QEMU

MULTIBOOT2_MAGIC equ 0xE85250D6
MULTIBOOT_ARCHITECTURE_I386 equ 0
HEADER_LENGTH equ multiboot_header_end - multiboot_header_start
CHECKSUM equ -(MULTIBOOT2_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + HEADER_LENGTH)

section .multiboot
align 8
multiboot_header_start:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT_ARCHITECTURE_I386
    dd HEADER_LENGTH
    dd CHECKSUM

    ; Tags
    ; Address Tag
    align 8
    dw 2                 ; type - address tag
    dw 0                 ; flags
    dd 24                ; size
    dd multiboot_header_start ; header_addr
    dd _start            ; load_addr
    dd load_end          ; load_end_addr
    dd bss_end           ; bss_end_addr

    ; Entry Tag
    align 8
    dw 3                 ; type - entry tag
    dw 0                 ; flags
    dd 12                ; size
    dd _start            ; entry_addr

    ; End Tag
    align 8
    dw 0                 ; type - end tag
    dw 0                 ; flags
    dd 8                 ; size
multiboot_header_end:

section .note.gnu.build-id
align 4
    dd 4                 ; name size
    dd 16                ; desc size
    dd 3                 ; type (NT_GNU_BUILD_ID)
    db "GNU", 0          ; name (null terminated)
    times 16 db 0        ; build-id placeholder

section .text
bits 32
global _start
extern kernel_main

_start:
    ; Set up stack
    mov esp, stack_top
    
    ; Pass multiboot info to kernel
    push ebx              ; multiboot info pointer
    push eax              ; multiboot magic value
    
    ; Call the kernel
    call kernel_main
    
    ; If we return, halt
    cli
    hlt
    jmp $                 ; Infinite loop

section .bss
align 4096
stack_bottom:
    resb 64*1024         ; 64K stack
stack_top:
load_end:
bss_end:
