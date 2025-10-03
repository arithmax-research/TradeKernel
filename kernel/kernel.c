#include "drivers/vga.h"
#include "mm/memory.h"
#include "arch/interrupts.h"
#include "shell.h"

// Simple string formatting functions
void print_hex(uint32_t value) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[11] = "0x";
    
    for (int i = 7; i >= 0; i--) {
        buffer[2 + (7 - i)] = hex_chars[(value >> (i * 4)) & 0xF];
    }
    buffer[10] = '\0';
    
    vga_write_string(buffer);
}

void print_dec(uint32_t value) {
    if (value == 0) {
        vga_putchar('0');
        return;
    }
    
    char buffer[12];
    int i = 0;
    
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    // Reverse the string
    for (int j = i - 1; j >= 0; j--) {
        vga_putchar(buffer[j]);
    }
}

// Basic memory detection (placeholder)
uint32_t detect_memory(void) {
    // This is a simple placeholder - in a real OS, you'd use BIOS int 0x15
    // or multiboot information to detect actual memory
    return 0x1000000; // Assume 16MB for now
}

// Kernel main function - called from bootloader
void kernel_main(void) {
    // Initialize VGA text mode
    vga_init();
    
    // Initialize memory management
    memory_init();
    
    // Initialize interrupts
    interrupts_init();
    
    // Set colors for a nice welcome screen
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_string("=================================================\n");
    vga_write_string("    TradeKernel OS - Built from Scratch v1.0    \n");
    vga_write_string("=================================================\n\n");
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write_string("Kernel successfully loaded and executing in 32-bit protected mode!\n\n");
    
    // Display system information
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_write_string("System Information:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_write_string("  Architecture: x86 (32-bit)\n");
    vga_write_string("  Memory detected: ");
    print_dec(detect_memory() / 1024);
    vga_write_string(" KB\n");
    vga_write_string("  Kernel heap: ");
    print_dec(get_total_memory() / 1024);
    vga_write_string(" KB\n");
    vga_write_string("  VGA Text Mode: 80x25\n\n");
    
    // Show kernel features
    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    vga_write_string("Kernel Features:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_write_string("  [OK] Bootloader transition\n");
    vga_write_string("  [OK] Protected mode\n");
    vga_write_string("  [OK] VGA text driver\n");
    vga_write_string("  [OK] Memory management\n");
    vga_write_string("  [OK] Interrupt handling\n");
    vga_write_string("  [  ] Scheduler\n\n");
    
    vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    vga_write_string("TradeKernel OS is ready for development!\n");
    vga_write_string("This is a minimal kernel that can be extended with trading algorithms.\n\n");
    
    vga_set_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    vga_write_string("Interactive shell enabled! Type 'help' for available commands.\n");
    vga_write_string("Timer interrupts are working in the background.\n\n");
    
    // Initialize and start the shell
    shell_init();
    
    // Keep the kernel alive
    while (1) {
        __asm__ volatile ("hlt");
    }
}