# TradeKernel OS Makefile

# Cross compiler settings
CC = gcc
AS = nasm
LD = ld

# Compiler flags
CFLAGS = -m32 -ffreestanding -fno-stack-protector -fno-builtin -nostdlib -nostdinc -Wall -Wextra -c
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T kernel/kernel.ld

# Directories
KERNEL_DIR = kernel
ARCH_DIR = $(KERNEL_DIR)/arch
DRIVERS_DIR = $(KERNEL_DIR)/drivers
MM_DIR = $(KERNEL_DIR)/mm
FS_DIR = $(KERNEL_DIR)/fs
BUILD_DIR = build

# Source files
BOOT_ASM = $(ARCH_DIR)/boot.asm
KERNEL_ENTRY_ASM = $(ARCH_DIR)/kernel_entry.asm
INTERRUPT_ASM = $(ARCH_DIR)/interrupt_handlers.asm
KERNEL_C = $(KERNEL_DIR)/kernel.c
VGA_C = $(DRIVERS_DIR)/vga.c
MEMORY_C = $(MM_DIR)/memory.c
PAGING_C = $(MM_DIR)/paging.c
PAGING_ASM = $(ARCH_DIR)/paging.asm
INTERRUPTS_C = $(ARCH_DIR)/interrupts.c
SHELL_C = $(KERNEL_DIR)/shell.c
FS_C = $(FS_DIR)/fs.c
DISK_C = $(FS_DIR)/disk.c

# Object files
BOOT_OBJ = $(BUILD_DIR)/boot.o
KERNEL_ENTRY_OBJ = $(BUILD_DIR)/kernel_entry.o
INTERRUPT_ASM_OBJ = $(BUILD_DIR)/interrupt_handlers.o
KERNEL_OBJ = $(BUILD_DIR)/kernel.o
VGA_OBJ = $(BUILD_DIR)/vga.o
MEMORY_OBJ = $(BUILD_DIR)/memory.o
PAGING_OBJ = $(BUILD_DIR)/paging.o
PAGING_ASM_OBJ = $(BUILD_DIR)/paging_asm.o
INTERRUPTS_OBJ = $(BUILD_DIR)/interrupts.o
SHELL_OBJ = $(BUILD_DIR)/shell.o
FS_OBJ = $(BUILD_DIR)/fs.o
DISK_OBJ = $(BUILD_DIR)/disk.o

# Target files
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
BOOT_BIN = $(BUILD_DIR)/boot.bin
OS_IMG = $(BUILD_DIR)/tradeos.img

# Default target
all: $(OS_IMG)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile bootloader
$(BOOT_BIN): $(BOOT_ASM) | $(BUILD_DIR)
	$(AS) -f bin $(BOOT_ASM) -o $(BOOT_BIN)

# Compile assembly files
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY_ASM) | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $(KERNEL_ENTRY_ASM) -o $(KERNEL_ENTRY_OBJ)

$(INTERRUPT_ASM_OBJ): $(INTERRUPT_ASM) | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $(INTERRUPT_ASM) -o $(INTERRUPT_ASM_OBJ)

# Compile C files
$(KERNEL_OBJ): $(KERNEL_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(KERNEL_DIR) $(KERNEL_C) -o $(KERNEL_OBJ)

$(VGA_OBJ): $(VGA_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(KERNEL_DIR) $(VGA_C) -o $(VGA_OBJ)

$(MEMORY_OBJ): $(MEMORY_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(KERNEL_DIR) $(MEMORY_C) -o $(MEMORY_OBJ)

$(PAGING_OBJ): $(PAGING_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(KERNEL_DIR) $(PAGING_C) -o $(PAGING_OBJ)

$(PAGING_ASM_OBJ): $(PAGING_ASM) | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $(PAGING_ASM) -o $(PAGING_ASM_OBJ)

$(INTERRUPTS_OBJ): $(INTERRUPTS_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(KERNEL_DIR) $(INTERRUPTS_C) -o $(INTERRUPTS_OBJ)

$(SHELL_OBJ): $(SHELL_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(KERNEL_DIR) $(SHELL_C) -o $(SHELL_OBJ)

$(FS_OBJ): $(FS_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(KERNEL_DIR) $(FS_C) -o $(FS_OBJ)

$(DISK_OBJ): $(DISK_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(KERNEL_DIR) $(DISK_C) -o $(DISK_OBJ)

# Link kernel
$(KERNEL_BIN): $(KERNEL_ENTRY_OBJ) $(KERNEL_OBJ) $(VGA_OBJ) $(MEMORY_OBJ) $(PAGING_OBJ) $(PAGING_ASM_OBJ) $(INTERRUPTS_OBJ) $(SHELL_OBJ) $(FS_OBJ) $(DISK_OBJ) $(INTERRUPT_ASM_OBJ)
	$(LD) $(LDFLAGS) $(KERNEL_ENTRY_OBJ) $(KERNEL_OBJ) $(VGA_OBJ) $(MEMORY_OBJ) $(PAGING_OBJ) $(PAGING_ASM_OBJ) $(INTERRUPTS_OBJ) $(SHELL_OBJ) $(FS_OBJ) $(DISK_OBJ) $(INTERRUPT_ASM_OBJ) -o $(KERNEL_BIN)

# Create OS image - hard disk format
$(OS_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	# Create a 10MB hard disk image
	dd if=/dev/zero of=$(OS_IMG) bs=1M count=10
	# Write bootloader to first sector (MBR)
	dd if=$(BOOT_BIN) of=$(OS_IMG) bs=512 count=1 conv=notrunc
	# Write kernel starting from second sector
	dd if=$(KERNEL_BIN) of=$(OS_IMG) bs=512 seek=1 conv=notrunc

# Run in QEMU - direct kernel loading (no bootloader needed)
run: $(KERNEL_BIN)
	qemu-system-i386 -kernel $(KERNEL_BIN) -m 16M

# Run with disk image (traditional boot)
run-disk: $(OS_IMG)
	qemu-system-i386 -drive format=raw,file=$(OS_IMG),if=ide -m 16M

# Run in QEMU with debugging
debug: $(OS_IMG)
	qemu-system-i386 -drive format=raw,file=$(OS_IMG),if=floppy -m 16M -s -S

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install build-essential nasm qemu-system-x86

.PHONY: all run debug clean install-deps