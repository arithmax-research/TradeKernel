#include "shell.h"
#include "drivers/vga.h"
#include "mm/memory.h"
#include "fs/fs.h"

static char command_buffer[MAX_COMMAND_LENGTH];
static int buffer_pos = 0;

// Built-in commands table
static shell_command_t commands[] = {
    {"help", "Show this help message", cmd_help},
    {"clear", "Clear the screen", cmd_clear},
    {"info", "Display system information", cmd_info},
    {"mem", "Show memory usage", cmd_mem},
    {"echo", "Print text to screen", cmd_echo},
    {"ls", "List directory contents", cmd_ls},
    {"mkdir", "Create directory", cmd_mkdir},
    {"touch", "Create empty file", cmd_touch},
    {"rm", "Remove file or directory", cmd_rm},
    {"reboot", "Restart the system", cmd_reboot},
    {NULL, NULL, NULL} // Terminator
};

void shell_init(void) {
    buffer_pos = 0;
    command_buffer[0] = '\0';
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write_string("$ ");
}

// Simple number printing function
static void print_dec(uint32_t value) {
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

// Simple string utilities
static int shell_strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

static int shell_strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

static void shell_strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

// Parse command line into arguments
static int parse_args(char* command_line, char* argv[]) {
    int argc = 0;
    char* token = command_line;
    
    while (*token && argc < MAX_ARGS - 1) {
        // Skip whitespace
        while (*token == ' ' || *token == '\t') token++;
        if (!*token) break;
        
        argv[argc++] = token;
        
        // Find end of token
        while (*token && *token != ' ' && *token != '\t') token++;
        if (*token) *token++ = '\0';
    }
    
    argv[argc] = NULL;
    return argc;
}

void shell_process_input(char c) {
    if (c == '\n' || c == '\r') {
        // Execute command
        vga_putchar('\n');
        command_buffer[buffer_pos] = '\0';
        
        if (buffer_pos > 0) {
            shell_execute_command(command_buffer);
        }
        
        // Reset for next command
        buffer_pos = 0;
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_write_string("$ ");
    } else if (c == '\b') {
        // Backspace
        if (buffer_pos > 0) {
            buffer_pos--;
            vga_putchar('\b');
        }
    } else if (c >= 32 && c <= 126) {
        // Printable character
        if (buffer_pos < MAX_COMMAND_LENGTH - 1) {
            command_buffer[buffer_pos++] = c;
            vga_putchar(c);
        }
    }
}

void shell_execute_command(const char* command_line) {
    char temp_buffer[MAX_COMMAND_LENGTH];
    char* argv[MAX_ARGS];
    int argc;
    
    if (shell_strlen(command_line) == 0) return;
    
    // Copy command line to temporary buffer for parsing
    shell_strcpy(temp_buffer, command_line);
    argc = parse_args(temp_buffer, argv);
    
    if (argc == 0) return;
    
    // Find and execute command
    for (int i = 0; commands[i].name != NULL; i++) {
        if (shell_strcmp(argv[0], commands[i].name) == 0) {
            commands[i].handler(argc, argv);
            return;
        }
    }
    
    // Command not found
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_write_string("Command not found: ");
    vga_write_string(argv[0]);
    vga_write_string("\nType 'help' for available commands.\n");
}

// Built-in command implementations
void cmd_help(int argc, char* argv[]) {
    (void)argc; (void)argv; // Suppress unused parameter warnings
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_string("TradeKernel OS - Available Commands:\n\n");
    
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    for (int i = 0; commands[i].name != NULL; i++) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_write_string(commands[i].name);
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        vga_write_string(" - ");
        vga_write_string(commands[i].description);
        vga_write_string("\n");
    }
    vga_write_string("\n");
}

void cmd_clear(int argc, char* argv[]) {
    (void)argc; (void)argv;
    vga_clear();
}

void cmd_info(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_string("=== System Information ===\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_write_string("OS: TradeKernel v1.0\n");
    vga_write_string("Architecture: x86 (32-bit)\n");
    vga_write_string("Memory: 16MB\n");
    vga_write_string("VGA Mode: 80x25 text\n");
    vga_write_string("Status: Running\n\n");
}

void cmd_mem(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    size_t total = get_total_memory();
    size_t free = get_free_memory();
    size_t used = total - free;
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_string("=== Memory Usage ===\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_write_string("Total Heap: ");
    print_dec(total / 1024);
    vga_write_string(" KB\n");
    vga_write_string("Used: ");
    print_dec(used / 1024);
    vga_write_string(" KB\n");
    vga_write_string("Free: ");
    print_dec(free / 1024);
    vga_write_string(" KB\n\n");
}

void cmd_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        vga_write_string(argv[i]);
        if (i < argc - 1) vga_write_string(" ");
    }
    vga_write_string("\n");
}

void cmd_reboot(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_write_string("Rebooting system...\n");
    
    // Simple reboot via keyboard controller
    uint8_t temp = 0xFE;
    __asm__ volatile ("outb %0, $0x64" : : "a"(temp));
    
    // If that doesn't work, halt
    vga_write_string("Reboot failed. System halted.\n");
    while (1) {
        __asm__ volatile ("hlt");
    }
}

void cmd_ls(int argc, char* argv[]) {
    const char* path = "/";  // Default to root directory
    
    if (argc > 1) {
        path = argv[1];
    }
    
    directory_entry_t entries[32];
    int count = fs_list_directory(path, entries, 32);
    
    if (count < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        if (count == FS_ERROR_NOT_FOUND) {
            vga_write_string("Directory not found: ");
            vga_write_string(path);
        } else if (count == FS_ERROR_INVALID) {
            vga_write_string("Not a directory: ");
            vga_write_string(path);
        } else {
            vga_write_string("Error reading directory");
        }
        vga_write_string("\n");
        return;
    }
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_string("Contents of ");
    vga_write_string(path);
    vga_write_string(":\n");
    
    if (count == 0) {
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        vga_write_string("  (empty directory)\n");
        return;
    }
    
    for (int i = 0; i < count; i++) {
        if (entries[i].file_type == FILE_TYPE_DIRECTORY) {
            vga_set_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
            vga_write_string("  [DIR]  ");
        } else {
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            vga_write_string("  [FILE] ");
        }
        vga_write_string(entries[i].name);
        vga_write_string("\n");
    }
}

void cmd_mkdir(int argc, char* argv[]) {
    if (argc < 2) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_string("Usage: mkdir <directory_name>\n");
        return;
    }
    
    const char* dir_path = argv[1];
    int result = fs_create_directory(dir_path);
    
    if (result == FS_SUCCESS) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_write_string("Directory created: ");
        vga_write_string(dir_path);
        vga_write_string("\n");
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        if (result == FS_ERROR_EXISTS) {
            vga_write_string("Directory already exists: ");
        } else if (result == FS_ERROR_NOT_FOUND) {
            vga_write_string("Parent directory not found: ");
        } else if (result == FS_ERROR_NO_SPACE) {
            vga_write_string("No space left on device: ");
        } else {
            vga_write_string("Failed to create directory: ");
        }
        vga_write_string(dir_path);
        vga_write_string("\n");
    }
}

void cmd_touch(int argc, char* argv[]) {
    if (argc < 2) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_string("Usage: touch <filename>\n");
        return;
    }
    
    const char* file_path = argv[1];
    int result = fs_create_file(file_path, FILE_TYPE_REGULAR);
    
    if (result == FS_SUCCESS) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_write_string("File created: ");
        vga_write_string(file_path);
        vga_write_string("\n");
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        if (result == FS_ERROR_EXISTS) {
            vga_write_string("File already exists: ");
        } else if (result == FS_ERROR_NOT_FOUND) {
            vga_write_string("Parent directory not found: ");
        } else if (result == FS_ERROR_NO_SPACE) {
            vga_write_string("No space left on device: ");
        } else {
            vga_write_string("Failed to create file: ");
        }
        vga_write_string(file_path);
        vga_write_string("\n");
    }
}

void cmd_rm(int argc, char* argv[]) {
    if (argc < 2) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_string("Usage: rm <filename>\n");
        return;
    }
    
    const char* file_path = argv[1];
    int result = fs_delete_file(file_path);
    
    if (result == FS_SUCCESS) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_write_string("File deleted: ");
        vga_write_string(file_path);
        vga_write_string("\n");
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        if (result == FS_ERROR_NOT_FOUND) {
            vga_write_string("File not found: ");
        } else {
            vga_write_string("Failed to delete file: ");
        }
        vga_write_string(file_path);
        vga_write_string("\n");
    }
}