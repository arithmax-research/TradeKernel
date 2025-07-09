#!/bin/bash
set -e

# Create a raw binary version of the kernel
x86_64-elf-objcopy -O binary build/tradekernel_kernel build/tradekernel.bin

echo "Created raw binary: build/tradekernel.bin"
