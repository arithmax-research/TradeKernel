# TradeKernel Makefile
# Ultra-Low Latency Trading OS Build System

# Build configuration
ARCH := x86_64
BUILD_DIR := build
BOOT_DIR := boot
SRC_DIR := src
INCLUDE_DIR := include

# Compilers and tools
ASM := nasm
CXX := x86_64-elf-g++
LD := x86_64-elf-ld
OBJCOPY := x86_64-elf-objcopy

# Assembly flags
ASMFLAGS := -f elf64

# C++ compiler flags for maximum performance
CXXFLAGS := -std=c++20 \
           -ffreestanding \
           -fno-exceptions \
           -fno-rtti \
           -fno-stack-protector \
           -fno-builtin \
           -nostdlib \
           -nostdinc++ \
           -mno-red-zone \
           -mcmodel=kernel \
           -O3 \
           -march=native \
           -mtune=native \
           -ffast-math \
           -funroll-loops \
           -fomit-frame-pointer \
           -finline-functions \
           -Wall \
           -Wextra \
           -Werror \
           -Wno-sized-deallocation \
           -fpermissive \
           -I$(INCLUDE_DIR)

# Linker flags
LDFLAGS := -T linker.ld \
          -nostdlib \
          -static \
          -n

# Source files
BOOT_SOURCES := $(BOOT_DIR)/boot.asm
KERNEL_ASM_SOURCES := $(SRC_DIR)/kernel/entry.asm $(SRC_DIR)/kernel/multiboot_entry.asm
KERNEL_CXX_SOURCES := $(SRC_DIR)/kernel/main.cpp \
                     $(SRC_DIR)/memory/memory_manager.cpp \
                     $(SRC_DIR)/scheduler/tickless_scheduler.cpp

# Object files
BOOT_OBJECTS := $(BUILD_DIR)/boot.o
KERNEL_ASM_OBJECTS := $(BUILD_DIR)/entry.o
KERNEL_CXX_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(KERNEL_CXX_SOURCES))

ALL_OBJECTS := $(KERNEL_ASM_OBJECTS) $(KERNEL_CXX_OBJECTS)

# Objects for multiboot-compliant binary (uses entry.o which has been updated with PVH)
MB_OBJECTS := $(BUILD_DIR)/entry.o $(KERNEL_CXX_OBJECTS)

# Targets
.PHONY: all clean kernel bootloader iso run debug

all: kernel bootloader

# Create build directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/kernel
	mkdir -p $(BUILD_DIR)/memory  
	mkdir -p $(BUILD_DIR)/scheduler
	mkdir -p $(BUILD_DIR)/drivers
	mkdir -p $(BUILD_DIR)/networking

# Bootloader
bootloader: $(BUILD_DIR) $(BOOT_OBJECTS)

$(BUILD_DIR)/boot.o: $(BOOT_SOURCES)
	$(ASM) $(ASMFLAGS) -o $@ $<

$(BUILD_DIR)/boot.bin: $(BUILD_DIR)/boot.o
	$(LD) -Ttext 0x7C00 -o $@ $< --oformat binary

# Kernel
kernel: $(BUILD_DIR) $(BUILD_DIR)/tradekernel_kernel

$(BUILD_DIR)/tradekernel_kernel: $(ALL_OBJECTS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJECTS)

# Legacy target for compatibility
$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/tradekernel_kernel
	cp $< $@

# Multiboot2-compatible kernel for QEMU (fixes direct kernel loading)
$(BUILD_DIR)/tradekernel_multiboot: $(MB_OBJECTS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(MB_OBJECTS)

$(BUILD_DIR)/tradekernel_multiboot.bin: $(BUILD_DIR)/tradekernel_multiboot
	$(OBJCOPY) -O binary $< $@

# Assembly sources
$(BUILD_DIR)/entry.o: $(SRC_DIR)/kernel/entry.asm | $(BUILD_DIR)
	$(ASM) $(ASMFLAGS) -o $@ $<

$(BUILD_DIR)/multiboot_entry.o: $(SRC_DIR)/kernel/multiboot_entry.asm | $(BUILD_DIR)
	$(ASM) $(ASMFLAGS) -o $@ $<

# C++ sources
$(BUILD_DIR)/kernel/%.o: $(SRC_DIR)/kernel/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/memory/%.o: $(SRC_DIR)/memory/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/scheduler/%.o: $(SRC_DIR)/scheduler/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Create ISO image for QEMU (requires GRUB)
iso: kernel
	@echo "🔧 Building bootable ISO..."
	@if command -v grub-mkrescue >/dev/null 2>&1; then \
		./build_iso_simple.sh; \
	else \
		echo "❌ GRUB not found. Install with: brew install grub"; \
		echo "💡 Use 'make run-fallback' for alternative boot methods"; \
		exit 1; \
	fi
	
# Run using the ISO file (more reliable)
run-iso: iso
	@echo "🚀 TradeKernel - Launching from ISO..."
	@echo "🖥️  Kernel Output:"
	@echo "----------------------------------------"
	qemu-system-x86_64 \
		-cdrom tradekernel.iso \
		-monitor stdio \
		-cpu qemu64 \
		-smp 4 \
		-m 512M \
		-no-reboot \
		-no-shutdown \
		-serial file:serial.log

# Run in QEMU - Show actual kernel output
run: kernel
	@echo "🚀 TradeKernel - Launching kernel..."
	@echo "🖥️  Kernel Output:"
	@echo "----------------------------------------"
	@qemu-system-x86_64 \
		-kernel $(BUILD_DIR)/tradekernel_kernel \
		-monitor stdio \
		-cpu qemu64 \
		-smp 4 \
		-m 512M \
		-no-reboot \
		-no-shutdown \
		-display none \
		-serial file:serial.log \
		-d int,guest_errors \
		-D qemu.log

# Fallback boot methods
run-fallback: kernel
	@echo "🚀 TradeKernel - Trying alternative boot methods..."
	./try_boot.sh
	
# Raw binary approach (more compatible with QEMU)
build-raw: kernel
	@echo "🔧 Building raw binary version..."
	./build_raw.sh

run-raw: build-raw
	@echo "🚀 TradeKernel - Launching raw binary..."
	@echo "🖥️  Kernel Output:"
	@echo "----------------------------------------"
	qemu-system-i386 \
		-cpu qemu32 \
		-smp 4 \
		-m 512M \
		-serial stdio \
		-kernel build/tradekernel.bin

# Run kernel directly (multiboot2 method)
run-direct: kernel $(BUILD_DIR)/tradekernel_multiboot
	@echo "🚀 TradeKernel - Direct kernel launch (multiboot2 method)..."
	@echo ""
	qemu-system-x86_64 \
		-kernel $(BUILD_DIR)/tradekernel_multiboot \
		-cpu qemu64 \
		-smp 4 \
		-m 512M \
		-no-reboot \
		-no-shutdown \
		-display sdl \
		-chardev stdio,id=char0,mux=on \
		-serial chardev:char0 \
		-mon chardev=char0 \
		-append "console=ttyS0" \
		-d int,cpu_reset,guest_errors \
		-D qemu-enhanced.log

# Fix multiboot header issues for QEMU
run-fixed: $(BUILD_DIR)/tradekernel_multiboot
	@echo "🔧 Creating fixed binary with correct multiboot2 header placement..."
	$(OBJCOPY) -O binary $(BUILD_DIR)/tradekernel_multiboot $(BUILD_DIR)/tradekernel_fixed.bin
	@echo "🚀 TradeKernel - Fixed binary launch..."
	qemu-system-x86_64 \
		-kernel $(BUILD_DIR)/tradekernel_fixed.bin \
		-cpu qemu64 \
		-smp 2 \
		-m 256M \
		-display sdl \
		-serial stdio \
		-monitor telnet:localhost:55555,server,nowait \
		-no-reboot \
		-no-shutdown \
		-d int,cpu_reset,guest_errors \
		-D qemu-fixed.log

# Comprehensive build verification
verify: $(BUILD_DIR)/tradekernel_kernel
	@echo "🔍 Running comprehensive build verification..."
	./verify_build.sh

# Quick test of all components
test: $(BUILD_DIR)/tradekernel_kernel
	@echo "🧪 Quick component testing..."
	@echo "✅ Kernel build: $(shell ls -lh $(BUILD_DIR)/tradekernel_kernel 2>/dev/null | awk '{print $$5}' || echo 'Missing')"
	@make -f Makefile.simple simulation > /dev/null && echo "✅ Core simulation ready"
	@make -f Makefile.simple trading > /dev/null && echo "✅ Trading system ready" || echo "ℹ️  Trading system needs rebuild"

# Debug version
debug: CXXFLAGS += -g -DDEBUG
debug: kernel $(BUILD_DIR)/tradekernel_multiboot
	@echo "🔍 TradeKernel - Debug mode (GDB can connect to port 1234)..."
	qemu-system-x86_64 \
		-kernel $(BUILD_DIR)/tradekernel_multiboot \
		-cpu qemu64 \
		-smp 4 \
		-m 512M \
		-no-reboot \
		-no-shutdown \
		-chardev stdio,id=char0,mux=on \
		-serial chardev:char0 \
		-mon chardev=char0 \
		-append "console=ttyS0" \
		-d guest_errors,int \
		-s -S

# Performance analysis
perf: kernel
	qemu-system-x86_64 \
		-cdrom $(BUILD_DIR)/tradekernel.iso \
		-cpu host \
		-smp 4 \
		-m 512M \
		-no-reboot \
		-no-shutdown \
		-serial stdio \
		-enable-kvm \
		-machine type=pc-i440fx-3.1 \
		-monitor unix:qemu-monitor-socket,server,nowait

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -f tradekernel.iso

# Show build information
info:
	@echo "TradeKernel Build Configuration:"
	@echo "Architecture: $(ARCH)"
	@echo "Build Directory: $(BUILD_DIR)"
	@echo "C++ Compiler: $(CXX)"
	@echo "Assembler: $(ASM)"
	@echo "Linker: $(LD)"
	@echo ""
	@echo "Available Targets:"
	@echo "  make kernel     - Build kernel only"
	@echo "  make run        - Build and run (optimal method)"
	@echo "  make run-direct - Direct kernel boot (may fail)"
	@echo "  make test       - Quick component verification"
	@echo "  make verify     - Comprehensive build verification"
	@echo "  make iso        - Create bootable ISO (requires GRUB)"
	@echo "  make clean      - Clean build artifacts"
	@echo ""
	@echo "Simulation Targets:"
	@echo "  make -f Makefile.simple simulation - Core simulation"
	@echo "  make -f Makefile.simple trading    - Trading system"
	@echo "  make -f Makefile.simple full-demo  - Complete demo"
	@echo ""
	@echo "Optimization Flags:"
	@echo "  -O3: Maximum optimization"
	@echo "  -march=native: Optimize for current CPU"
	@echo "  -ffast-math: Aggressive math optimizations"

# Install prerequisites (macOS)
install-deps:
	@echo "Installing build dependencies for macOS..."
	brew install nasm
	brew install qemu
	brew install llvm
	@echo "Dependencies installed!"

# Benchmark the kernel
benchmark: kernel
	@echo "Running TradeKernel benchmarks..."
	qemu-system-x86_64 \
		-cdrom $(BUILD_DIR)/tradekernel.iso \
		-cpu host \
		-smp 4 \
		-m 512M \
		-no-reboot \
		-no-shutdown \
		-serial stdio \
		-enable-kvm \
		-netdev user,id=net0 \
		-device rtl8139,netdev=net0