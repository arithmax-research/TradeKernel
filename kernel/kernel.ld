ENTRY(_start)

SECTIONS
{
    . = 0x10000;
    
    .multiboot :
    {
        *(.multiboot)
    }
    
    .text :
    {
        *(.text)
    }
    
    .rodata :
    {
        *(.rodata*)
    }
    
    .data :
    {
        *(.data)
    }
    
    .bss :
    {
        *(COMMON)
        *(.bss)
    }
}