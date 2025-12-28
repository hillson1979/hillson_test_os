#!/bin/bash
# GDB调试脚本 - 用于调试用户进程启动

cd /mnt/f/hillson_test_os/hillson_test_os

echo "=== QEMU GDB调试模式启动脚本 ==="
echo ""
echo "步骤1: 启动QEMU (在后台运行,等待GDB连接)"
echo ""

qemu-system-i386 \
    -cdrom os.iso \
    -m 4096 \
    -s \
    -S \
    -serial file:debug_output.txt \
    -nographic \
    -monitor null &

QEMU_PID=$!
echo "QEMU PID: $QEMU_PID"
echo ""

echo "步骤2: 创建GDB命令文件"
cat > gdb_commands.txt << 'GDBCMDS'
# 加载符号文件
symbol-file kernel.bin

# 设置架构
set architecture i8086

# 连接到QEMU
target remote localhost:1234

# 设置断点
break task_to_user_mode
break user_task_main
break start_task_user

# 显示当前位置
info breakpoints

# 继续执行
continue
GDBCMDS

echo ""
echo "步骤3: 启动GDB"
echo ""
echo "运行以下命令启动GDB:"
echo "  gdb -x gdb_commands.txt kernel.bin"
echo ""
echo "或者在GDB中手动执行:"
echo "  $ gdb kernel.bin"
echo "  (gdb) target remote :1234"
echo "  (gdb) break task_to_user_mode"
echo "  (gdb) continue"
echo ""
echo "步骤4: 调试完成后,按Ctrl+C停止QEMU"
echo "  kill $QEMU_PID"
echo ""
echo "调试输出将保存到: debug_output.txt"
echo ""
