#!/bin/bash
# 完整测试 ISO 并捕获所有输出

cd /mnt/f/hillson_test_os/hillson_test_os

echo "========================================"
echo "开始完整测试 - 输出到 full_test_output.txt"
echo "========================================"

qemu-system-i386 -cdrom os.iso -m 4096 -nographic -serial file:full_test_output.txt &
QEMU_PID=$!

echo "QEMU PID: $QEMU_PID"
echo "等待 20 秒..."
sleep 20

echo "终止 QEMU..."
kill $QEMU_PID 2>/dev/null

echo ""
echo "========================================"
echo "测试输出："
echo "========================================"
cat full_test_output.txt

echo ""
echo "========================================"
echo "测试完成"
echo "========================================"
