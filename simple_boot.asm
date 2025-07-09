; Simple bootloader for TradeKernel
; This creates a minimal bootloader that can load our kernel

[org 0x7C00]
[bits 16]

; Boot sector code
start:
    ; Set up segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    
    ; Print loading message
    mov si, msg_loading
    call print_string
    
    ; Load kernel from sector 2
    mov bx, 0x8000      ; Load address
    mov dh, 0           ; Head 0
    mov dl, 0x80        ; Drive 0x80 (first hard disk)
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Sector 2
    mov al, 64          ; Number of sectors to read
    mov ah, 0x02        ; BIOS read sectors function
    int 0x13
    
    jc disk_error
    
    ; Switch to protected mode
    call enable_a20
    lgdt [gdt_descriptor]
    
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    jmp 0x08:protected_mode
    
disk_error:
    mov si, msg_disk_error
    call print_string
    hlt

print_string:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string
.done:
    ret

enable_a20:
    ; Enable A20 line via keyboard controller
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

[bits 32]
protected_mode:
    ; Set up protected mode segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    
    ; Jump to kernel
    jmp 0x8000

; GDT
gdt_start:
    ; Null descriptor
    dq 0
    
    ; Code segment
    dw 0xFFFF       ; Limit low
    dw 0x0000       ; Base low
    db 0x00         ; Base middle
    db 10011010b    ; Access byte
    db 11001111b    ; Granularity
    db 0x00         ; Base high
    
    ; Data segment
    dw 0xFFFF       ; Limit low
    dw 0x0000       ; Base low
    db 0x00         ; Base middle
    db 10010010b    ; Access byte
    db 11001111b    ; Granularity
    db 0x00         ; Base high
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; Messages
msg_loading db 'Loading TradeKernel...', 13, 10, 0
msg_disk_error db 'Disk read error!', 13, 10, 0

; Pad to 510 bytes and add boot signature
times 510-($-$$) db 0
dw 0xAA55
