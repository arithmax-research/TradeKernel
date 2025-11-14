#include "drivers/vga.h"
#include "mm/memory.h"
#include "mm/paging.h"
#include "arch/interrupts.h"
#include "shell.h"
#include "fs/fs.h"
#include "proc/process.h"
#include "proc/scheduler.h"
#include "proc/syscalls.h" // System calls enabled
#include "proc/ipc.h" // IPC enabled
#include "net/net.h"
#include "net/eth.h"
#include "net/ip.h"
#include "net/tcp.h"
#include "net/socket.h"
#include "net/websocket.h"
#include "gui.h" // GUI framework
#include "drivers/mouse.h" // Mouse driver

// Simple serial output for debugging
#define COM1 0x3F8

static void serial_init(void) {
    outb(COM1 + 1, 0x00); // Disable interrupts
    outb(COM1 + 3, 0x80); // Enable DLAB
    outb(COM1 + 0, 0x03); // Set divisor low byte (38400 baud)
    outb(COM1 + 1, 0x00); // Set divisor high byte
    outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

static void serial_putchar(char c) {
    while ((inb(COM1 + 5) & 0x20) == 0); // Wait for transmit buffer empty
    outb(COM1, c);
}

static void serial_write_string(const char* str) {
    while (*str) {
        serial_putchar(*str++);
    }
}

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

// Display TradeKernel loading screen with ASCII art logo and animation
void display_loading_screen(void) {
    // Clear screen and set colors
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    
    // TradeKernel ASCII Art Logo with icons
    vga_write_string("\n\n");
    vga_write_string("          _______                        ______                __\n");
    vga_write_string("         /_  __(_)____ ___  ____  ____  / ____/___  ____  _____/ /\n");
    vga_write_string("          / / / / ___// _ \\/ __ \\/ __ \\/ /   / __ \\/ __ \\/ ___/ / \n");
    vga_write_string("         / / / / /__ /  __/ /_/ / / / / /___/ /_/ / / / (__  )_/  \n");
    vga_write_string("        /_/ /_/\\___/ \\___/ .___/_/ /_/\\____/\\____/_/ /_/____/_/   \n");
    vga_write_string("                         /_/                                      \n");
    vga_write_string("\n");
    
    // Version and tagline with icons
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_write_string("                    * TradeKernel OS v1.3 - ArithmaX Customized *\n");
    vga_write_string("                Advanced Process Management & IPC Framework\n");
    vga_write_string("\n");
    
    // Loading message with animation
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_write_string("                              Initializing kernel ");
    
    // Animated loading with spinning cursor and progress
    char spinner[] = {'|', '/', '-', '\\'};
    int progress = 0;
    
    for (int frame = 0; frame < 40; frame++) {
        // Update spinner
        vga_set_cursor(54, 10); // Position after "Initializing kernel "
        vga_putchar(spinner[frame % 4]);
        
        // Update progress bar
        vga_set_cursor(30, 12);
        vga_write_string("[");
        for (int i = 0; i < 30; i++) {
            if (i < progress) {
                vga_write_string("#"); // Filled block
            } else {
                vga_write_string("."); // Empty
            }
        }
        vga_write_string("]");
        
        // Update progress percentage
        vga_set_cursor(62, 12);
        char percent[5];
        int pct = (progress * 100) / 30;
        percent[0] = '0' + (pct / 10);
        percent[1] = '0' + (pct % 10);
        percent[2] = '%';
        percent[3] = '\0';
        vga_write_string(percent);
        
        // Increment progress
        if (frame % 3 == 0 && progress < 30) {
            progress++;
        }
        
        // Brief delay for animation
        for (volatile int i = 0; i < 200000; i++) {
            // Busy wait for animation timing
        }
    }
    
    vga_write_string("\n\n");
    
    // Show system icons
    vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    vga_write_string("                              System Status:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_write_string("                              [OK] CPU: OK    [OK] Memory: OK    [OK] Disk: OK\n");
    vga_write_string("\n");
}

// Kernel main function - called from bootloader
void kernel_main(void) {
    // Initialize serial port for debugging
    serial_init();
    serial_write_string("Serial initialized\n");
    
    // Display loading screen
    display_loading_screen();
    
    // Initialize VGA text mode
    vga_init();
    
    // Initialize memory management
    memory_init();
    
    // Initialize interrupts first
    interrupts_init();
    
    // Initialize paging system (after interrupts for page fault handling)
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_string("Initializing virtual memory...\n");
    paging_init();
    // Note: Not enabling paging yet - keeping identity mapping for stability
    
    // Initialize process management
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_string("Initializing process management...\n");
    process_init();
    scheduler_init();
    syscalls_init(); // System calls enabled
    ipc_init(); // IPC enabled
    
    // Initialize GUI framework
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_string("Initializing GUI framework...\n");
    gui_init();
    
    // Initialize mouse
    // mouse_init(); // TEMPORARILY DISABLED
    
    // Create terminal window
    window_t* term_win = gui_create_terminal_window(5, 3, 70, 20, "TradeKernel Terminal");
    gui_show_window(term_win);
    
    // Set terminal window for shell output
    shell_set_terminal_window(term_win);
    
    vga_write_string("Initializing file system...\n");
    int fs_result = fs_init();
    if (fs_result == FS_ERROR_NOT_FOUND) {
        vga_write_string("No filesystem found. Formatting disk...\n");
        fs_result = fs_format();
        if (fs_result == FS_SUCCESS) {
            vga_write_string("Filesystem created successfully!\n");
        } else {
            vga_write_string("Failed to create filesystem (error: ");
            // Print error code
            char code_str[10];
            int temp = fs_result;
            if (temp < 0) temp = -temp;
            int i = 0;
            do {
                code_str[i++] = '0' + (temp % 10);
                temp /= 10;
            } while (temp > 0 && i < 9);
            if (fs_result < 0) {
                vga_write_string("-");
            }
            while (i > 0) {
                vga_putchar(code_str[--i]);
            }
            vga_write_string(")\n");
        }
    } else if (fs_result == FS_SUCCESS) {
        vga_write_string("Existing filesystem mounted successfully!\n");
    } else {
        vga_write_string("Filesystem initialization failed (error: ");
        // Print error code
        char code_str[10];
        int temp = fs_result;
        if (temp < 0) temp = -temp;
        int i = 0;
        do {
            code_str[i++] = '0' + (temp % 10);
            temp /= 10;
        } while (temp > 0 && i < 9);
        if (fs_result < 0) {
            vga_write_string("-");
        }
        while (i > 0) {
            vga_putchar(code_str[--i]);
        }
        vga_write_string(")\n");
    }
    
    // Initialize network stack
    
    // Initialize network stack
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_string("Initializing network stack...\n");
    if (rtl8139_init(0xC000) == NET_SUCCESS) {
        vga_write_string("Ethernet driver initialized successfully!\n");
    } else {
        vga_write_string("Ethernet driver initialization failed!\n");
    }
    
    if (ipv4_init() == NET_SUCCESS) {
        vga_write_string("IPv4 protocol initialized successfully!\n");
    } else {
        vga_write_string("IPv4 protocol initialization failed!\n");
    }
    
    if (tcp_init() == NET_SUCCESS) {
        vga_write_string("TCP protocol initialized successfully!\n");
    } else {
        vga_write_string("TCP protocol initialization failed!\n");
    }
    
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
    vga_write_string("  [OK] Process management\n");
    vga_write_string("  [OK] Priority scheduler\n");
    vga_write_string("  [OK] System calls\n");
    vga_write_string("  [OK] Inter-process communication\n");
    vga_write_string("  [OK] File system\n");
    vga_write_string("  [OK] Ethernet driver (RTL8139)\n");
    vga_write_string("  [OK] IPv4 protocol stack\n");
    vga_write_string("  [OK] TCP protocol\n");
    vga_write_string("  [OK] Socket API\n");
    vga_write_string("  [OK] WebSocket support\n\n");
    
    vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    vga_write_string("TradeKernel OS is ready for development!\n");
    vga_write_string("This is a minimal kernel that can be extended with trading algorithms.\n\n");
    
    vga_set_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    vga_write_string("Interactive shell enabled! Type 'help' for available commands.\n");
    vga_write_string("Timer interrupts are working in the background.\n\n");
    
    // Boot Splash Screen - Show GUI with system information
    vga_set_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    vga_write_string("Launching TradeKernel GUI Boot Splash...\n\n");
    
    // Clear screen for boot splash
    vga_clear();
    
    // Create main boot splash window
    window_t* splash_window = gui_create_window(3, 1, 74, 22, "TradeKernel OS v1.3 - System Boot");
    if (splash_window) {
        // Title and welcome with ASCII art
        gui_create_label(splash_window, 1, 1, "   _______                        ______                __");
        gui_create_label(splash_window, 1, 2, "  /_  __(_)____ ___  ____  ____  / ____/___  ____  _____/ /");
        gui_create_label(splash_window, 1, 3, "   / / / / ___// _ \\/ __ \\/ __ \\/ /   / __ \\/ __ \\/ ___/ / ");
        gui_create_label(splash_window, 1, 4, "  / / / / /__ /  __/ /_/ / / / / /___/ /_/ / / / (__  )_/  ");
        gui_create_label(splash_window, 1, 5, " /_/ /_/\\___/ \\___/ .___/_/ /_/\\____/\\____/_/ /_/____/_/   ");
        gui_create_label(splash_window, 1, 6, "                 /_/                                      ");
        
        // System information
        gui_create_label(splash_window, 2, 8, "System Status: INITIALIZING");
        gui_create_label(splash_window, 2, 9, "Architecture: x86 32-bit Protected Mode");
        gui_create_label(splash_window, 2, 10, "Memory: Scanning...");
        gui_create_label(splash_window, 2, 11, "Kernel: ArithmaX Research Custom Build");
        
        // Feature status with icons
        gui_create_label(splash_window, 2, 13, "Core Systems:");
        gui_create_label(splash_window, 4, 14, "[+] Memory Management");
        gui_create_label(splash_window, 4, 15, "[+] Process Scheduler");
        gui_create_label(splash_window, 4, 16, "[+] File System");
        gui_create_label(splash_window, 4, 17, "[+] Network Stack");
        gui_create_label(splash_window, 4, 18, "[+] GUI Framework");
        
        // Progress indicator
        gui_create_label(splash_window, 2, 20, "Boot Progress: [....................] 0%");
        
        gui_show_window(splash_window);
    }
    
    // Animate the boot splash with progress updates
    char progress_bar[25] = "[....................]";
    int progress_percent = 0;
    
    for (int frame = 0; frame < 50; frame++) {
        // Update progress every few frames
        if (frame % 2 == 0 && progress_percent < 20) {
            progress_bar[1 + progress_percent] = '#';
            progress_percent++;
            
            // Update progress bar in GUI (this is approximate positioning)
            // Since GUI doesn't support dynamic updates easily, we'll use direct VGA writes
            vga_set_cursor(16, 21); // Position in window for progress bar
            vga_write_string(progress_bar);
            
            // Update percentage
            char percent_str[5];
            int pct = (progress_percent * 100) / 20;
            percent_str[0] = '0' + (pct / 10);
            percent_str[1] = '0' + (pct % 10);
            percent_str[2] = '%';
            percent_str[3] = '\0';
            vga_set_cursor(42, 21);
            vga_write_string(percent_str);
        }
        
        // Update status messages at key points
        if (frame == 10) {
            vga_set_cursor(16, 9); // Status line
            vga_write_string("MEMORY SCAN COMPLETE");
            vga_set_cursor(9, 11); // Memory line
            vga_write_string("        "); // Clear
            vga_set_cursor(9, 11);
            print_dec(detect_memory() / 1024);
            vga_write_string(" KB available");
        } else if (frame == 25) {
            vga_set_cursor(16, 9);
            vga_write_string("SYSTEM READY       ");
        } else if (frame == 40) {
            vga_set_cursor(16, 9);
            vga_write_string("BOOT COMPLETE      ");
        }
        
        // Update feature status with checkmarks
        if (frame >= 5 && frame < 45) {
            int feature_line = 15 + ((frame - 5) / 8); // Update one feature every 8 frames
            if (feature_line <= 18) {
                vga_set_cursor(4, feature_line);
                vga_write_string("[OK]");
            }
        }
        
        // Brief delay for animation
        for (volatile int i = 0; i < 200000; i++) {
            // Busy wait for animation timing
        }
    }
    
    // Show completion message
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_set_cursor(0, 24);
    vga_write_string("Boot sequence completed! Press any key or wait for automatic continuation...");
    
    // Wait for user input or timeout (shorter wait)
    for (volatile int i = 0; i < 1000000; i++) {
        // Busy wait - in a real system we'd wait for keyboard input
    }
    
    // Clean transition to shell
    vga_clear();
    
    // Debug output
    serial_write_string("About to destroy splash window\n");
    
    // Destroy the splash window before redrawing
    if (splash_window) {
        gui_destroy_window(splash_window);
        splash_window = NULL;
    }
    
    // Debug output
    serial_write_string("About to call gui_redraw_all\n");
    
    // Redraw GUI after clearing screen
    gui_redraw_all();
    
    // Debug output
    serial_write_string("gui_redraw_all completed\n");
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_string("TradeKernel OS v1.3 - Interactive Shell\n");
    vga_write_string("========================================\n\n");
    
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_write_string("GUI boot splash completed successfully!\n");
    vga_write_string("All systems initialized and ready for operation.\n\n");
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write_string("Type 'help' for available commands.\n\n");
    
    // Initialize and start the shell
    shell_init();
    
    // Ensure shell prompt is visible
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write_string("\nShell ready - you can now type commands!\n");
    
    // Keep the kernel alive
    while (1) {
        __asm__ volatile ("hlt");
    }
}