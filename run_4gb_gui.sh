#!/bin/bash
# Test kernel with 4GB memory configuration (GUI mode)

echo "========================================="
echo "Testing Kernel with 4GB Memory (GUI)"
echo "========================================="

# Build kernel first
echo "Building kernel..."
make clean
make

if [ ! -f kernel.bin ]; then
    echo "ERROR: kernel.bin not found!"
    exit 1
fi

echo "========================================="
echo "Running QEMU with 4GB memory (GUI)..."
echo "========================================="
echo "Expected behavior:"
echo "  - Detect ~3070MB available physical memory"
echo "  - Buddy System manages ~3035MB (777,088 pages)"
echo "  - System startup may take 20-30 seconds"
echo "  - Press Ctrl+C to stop QEMU"
echo "========================================="

# Run QEMU with 4GB memory with graphical interface
qemu-system-i386 \
    -kernel kernel.bin \
    -m 4096

echo "========================================="
echo "Test completed"
echo "========================================="
