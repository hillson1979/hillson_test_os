#!/bin/bash
cd /mnt/f/hillson_test_os/hillson_test_os
qemu-system-i386 -cdrom os.iso -m 4096 -serial file:shell_test_output.txt -nographic -monitor none -device VGA &
QEMU_PID=$!
sleep 15
kill $QEMU_PID 2>/dev/null
cat shell_test_output.txt
