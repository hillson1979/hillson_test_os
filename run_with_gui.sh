#!/bin/bash
cd /mnt/f/hillson_test_os/hillson_test_os

echo "=== Starting QEMU with GUI ==="
echo "Press Ctrl+Alt+G to release mouse capture"
echo "Press Ctrl+Alt+F to toggle fullscreen"
echo ""

qemu-system-i386 \
    -cdrom os.iso \
    -m 4096 \
    -serial stdio \
    2>&1 &

QEMU_PID=$!
echo "QEMU PID: $QEMU_PID"
echo ""
echo "QEMU started. Close the QEMU window to stop."
wait $QEMU_PID
echo "QEMU closed."
