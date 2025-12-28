#!/bin/bash
# 测试 ISO 并捕获输出到文件

cd /mnt/f/hillson_test_os/hillson_test_os

echo "正在启动 QEMU 测试 ISO..."
echo "输出将保存到 iso_test_output.txt"

qemu-system-i386 -cdrom os.iso -m 4096 -serial file:iso_test_output.txt -nographic -monitor none &
QEMU_PID=$!

# 等待 15 秒
sleep 15

# 终止 QEMU
kill $QEMU_PID 2>/dev/null

# 显示输出
echo ""
echo "========== 测试输出 =========="
cat iso_test_output.txt
echo "========== 输出结束 =========="
