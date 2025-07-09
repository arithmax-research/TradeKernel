#!/bin/bash
set -e

# Create ISO structure
mkdir -p iso_root/boot/grub

# Copy kernel
cp build/tradekernel_kernel iso_root/boot/kernel.bin

# Create GRUB config
cat > iso_root/boot/grub/grub.cfg << 'EOF'
set timeout=0
set default=0

menuentry "TradeKernel" {
    multiboot /boot/kernel.bin
    boot
}
EOF

# Create ISO (requires grub-mkrescue)
grub-mkrescue -o tradekernel.iso iso_root

echo "Created tradekernel.iso"
