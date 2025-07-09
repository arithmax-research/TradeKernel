#!/bin/bash

# TradeKernel ISO Builder
# Creates a bootable ISO image with GRUB2 bootloader

set -e

echo "Building TradeKernel ISO image..."

# Create build directory structure
mkdir -p build/iso/boot/grub

# Copy kernel
if [ ! -f "build/tradekernel_kernel" ]; then
    echo "Error: Kernel not found. Run 'make kernel' first."
    exit 1
fi

cp build/tradekernel_kernel build/iso/boot/

# Copy GRUB configuration
cp grub.cfg build/iso/boot/grub/

# Check if grub-mkrescue is available
if ! command -v grub-mkrescue &> /dev/null; then
    echo "Error: grub-mkrescue not found."
    echo "On macOS, install with: brew install grub"
    echo "On Ubuntu/Debian: sudo apt-get install grub-pc-bin grub-common"
    echo "On CentOS/RHEL: sudo yum install grub2-tools"
    exit 1
fi

# Create ISO
echo "Creating ISO image..."
grub-mkrescue -o build/tradekernel.iso build/iso/

echo "ISO created: build/tradekernel.iso"
echo "To run with QEMU: qemu-system-x86_64 -cdrom build/tradekernel.iso"
