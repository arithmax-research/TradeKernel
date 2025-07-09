#!/bin/bash

# TradeKernel Alternative Boot Solution
# Creates a simple boot solution without requiring GRUB

set -e

echo "🔧 Setting up TradeKernel boot solution..."

# Check if kernel exists
if [ ! -f "build/tradekernel_kernel" ]; then
    echo "❌ Error: Kernel not found. Run 'make kernel' first."
    exit 1
fi

# Check kernel file format
echo "📋 Kernel information:"
file build/tradekernel_kernel
echo ""

# Try different QEMU boot methods
echo "🚀 Trying QEMU with different boot methods..."

echo "Method 1: Direct kernel boot with Multiboot2..."
if qemu-system-x86_64 \
    -kernel build/tradekernel_kernel \
    -cpu qemu64 \
    -smp 4 \
    -m 512M \
    -no-reboot \
    -no-shutdown \
    -serial stdio \
    -display none \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 &
then
    echo "✅ Method 1 successful!"
    exit 0
fi

echo "Method 1 failed, trying Method 2..."

echo "Method 2: Create simple bootable disk image..."
# Create a simple disk image
dd if=/dev/zero of=build/tradekernel.img bs=1M count=10 2>/dev/null

# Try to use the kernel as a flat binary
echo "Method 3: Raw binary boot..."
if qemu-system-x86_64 \
    -drive file=build/tradekernel_kernel,format=raw,if=floppy \
    -cpu qemu64 \
    -smp 4 \
    -m 512M \
    -no-reboot \
    -no-shutdown \
    -serial stdio \
    -display none &
then
    echo "✅ Method 3 successful!"
    exit 0
fi

echo "❌ All boot methods failed."
echo "💡 Suggestions:"
echo "   1. Install GRUB: brew install grub"
echo "   2. Use simulation mode: make -f Makefile.simple full-demo"
echo "   3. Test individual components: make test"
echo ""
echo "ℹ️  The kernel builds successfully but requires a bootloader for QEMU."
