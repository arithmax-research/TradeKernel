section .data
	hello:     db 'Hello, World!',10    ; 'Hello, World!' plus a linefeed character
	helloLen:  equ $-hello             ; Length of the 'Hello world!' string

section .text
	global _main

_main:
	mov rax, 0x2000004    ; macOS write syscall
	mov rdi, 1            ; File descriptor 1 - standard output
	lea rsi, [rel hello]  ; Load effective address of hello
	mov rdx, helloLen     ; helloLen is a constant
	syscall               ; Call the kernel
	mov rax, 0x2000001    ; macOS exit syscall
	mov rdi, 0            ; Exit with return "code" of 0 (no error)
	syscall