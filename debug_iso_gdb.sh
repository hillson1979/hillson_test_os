#!/bin/bash
# 使用 GDB 调试 ISO 启动

cd /mnt/f/hillson_test_os/hillson_test_os

echo "启动 QEMU 等待 GDB 连接..."
qemu-system-i386 -cdrom os.iso -m 4096 -nographic -s -S &
QEMU_PID=$!

echo "QEMU PID: $QEMU_PID"
echo "GDB 监听端口: localhost:1234"
echo ""
echo "请在另一个终端运行："
echo "  gdb kernel.bin"
echo "  (gdb) target remote localhost:1234"
echo "  (gdb) break load_module_to_user"
echo "  (gdb) continue"
echo ""
echo "等待 60 秒..."
sleep 60

kill $QEMU_PID 2>/dev/null
echo "调试会话结束"
