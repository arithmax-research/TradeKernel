section .data
    hello:     db 'Hello, World!',10
    helloLen:  equ $-hello

section .text
    global _start

_start:
    mov eax,4            ; sys_write
    mov ebx,1            ; stdout
    mov ecx,hello
    mov edx,helloLen
    int 80h              ; Linux syscall
    mov eax,1            ; sys_exit
    mov ebx,0
    int 80h