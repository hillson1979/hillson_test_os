#!/bin/bash
cd /mnt/f/hillson_test_os/hillson_test_os

echo "=== Starting QEMU test ==="
echo "ISO: os.iso"
echo "Memory: 4096 MB"
echo ""

qemu-system-i386 \
    -cdrom os.iso \
    -m 4096 \
    -serial file:qemu_test_output.txt \
    -nographic \
    -monitor none \
    -device VGA \
    -d int,cpu_reset \
    2>&1 &

QEMU_PID=$!
echo "QEMU PID: $QEMU_PID"
echo "Waiting 15 seconds..."
sleep 15

echo "=== Killing QEMU ==="
kill $QEMU_PID 2>/dev/null
wait $QEMU_PID 2>/dev/null

echo ""
echo "=== Output ==="
cat qemu_test_output.txt

echo ""
echo "=== Test complete ==="
