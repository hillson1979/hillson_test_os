#!/bin/bash
# 测试kernel.bin - 不使用ISO,直接用multiboot

cd /mnt/f/hillson_test_os/hillson_test_os

echo "=== Testing kernel.bin directly ==="
echo "Kernel: kernel.bin"
echo "Module: test/test.elf"
echo ""

# 清理旧的输出
rm -f test_output.txt

# 启动QEMU,直接加载kernel.bin和模块
qemu-system-i386 \
    -kernel kernel.bin \
    -m 4096 \
    -initrd test/test.elf \
    -append "user_program" \
    -serial file:test_output.txt \
    -nographic \
    -display none \
    -monitor none \
    2>&1 &

QEMU_PID=$!
echo "QEMU PID: $QEMU_PID"
echo "Waiting 15 seconds for kernel to boot..."
sleep 15

# 停止QEMU
echo ""
echo "=== Stopping QEMU ==="
kill $QEMU_PID 2>/dev/null
wait $QEMU_PID 2>/dev/null

# 显示输出
echo ""
echo "=== Kernel Output ==="
if [ -f test_output.txt ]; then
    cat test_output.txt
else
    echo "No output file generated"
fi

echo ""
echo "=== Test complete ==="
