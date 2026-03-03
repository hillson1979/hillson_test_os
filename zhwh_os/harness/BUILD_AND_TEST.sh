#!/bin/bash
# Build and Test Script for zhwh_os
# Usage: wsl bash -c "cd /mnt/f/hillson_test_os/zhwh_os && make clean && make && ./zh.sh"

set -e

echo "========================================"
echo "  zhwh_os Build and Test Script"
echo "========================================"
echo ""

cd /mnt/f/hillson_test_os/zhwh_os

echo "[1/5] Cleaning previous build..."
make clean

echo ""
echo "[2/5] Generating task offsets..."
make gen-offsets

echo ""
echo "[3/5] Building kernel..."
make

if [ -f kernel.bin ]; then
    echo "  ✓ kernel.bin built successfully"
else
    echo "  ✗ kernel.bin build FAILED"
    exit 1
fi

echo ""
echo "[4/5] Building user programs..."
cd test
if [ -f Makefile ]; then
    make syscall_test.elf 2>/dev/null || echo "  Note: syscall_test.elf build skipped (requires gcc)"
    make lvglanet.elf 2>/dev/null || echo "  Note: lvglanet.elf build skipped"
    cd ..
else
    echo "  Note: test/Makefile not found"
    cd ..
fi

echo ""
echo "[5/5] Creating ISO image..."
./zh.sh

if [ -f os.iso ]; then
    echo "  ✓ os.iso created successfully"
    echo ""
    echo "========================================"
    echo "  Build Complete!"
    echo "========================================"
    echo ""
    echo "To run with QEMU:"
    echo "  qemu-system-i386 -cdrom os.iso -vga std -display sdl"
    echo ""
    echo "With serial output:"
    echo "  qemu-system-i386 -cdrom os.iso -vga std -display sdl -serial file:output.txt"
    echo ""
    echo "Boot entries:"
    echo "  - My OS - File System Test"
    echo "  - My OS - Network Shell"
    echo "  - My OS - GUI Shell"
    echo "  - My OS - LVGL Graphics Test"
    echo "  - My OS - LVGL + Network Test (Video Player)"
    echo "  - My OS - System Call Test"
    echo ""
else
    echo "  ✗ os.iso creation FAILED"
    exit 1
fi
