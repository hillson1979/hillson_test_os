#!/bin/bash
# zh.sh - 重新编译内核并更新ISO

set -e  # 遇到错误立即退出

echo "========================================="
echo "开始重新编译内核并更新ISO"
echo "========================================="

# 进入内核源码目录
cd hillson_test_os

echo "[1/4] 清理旧文件..."
make clean

echo "[2/4] 编译内核..."
make

echo "[3/4] 更新ISO boot目录..."
# 复制内核到ISO目录
cp kernel.bin iso/boot/

# 如果用户进程存在，也复制过去
if [ -f ../test/shell_demo.elf ]; then
    cp ../test/shell_demo.elf iso/boot/
    echo "  已复制 shell_demo.elf"
fi

echo "[4/4] 生成ISO文件..."
# 生成ISO
grub-mkrescue -o ../os.iso iso

echo "========================================="
echo "✓ ISO更新完成！"
echo "========================================="
echo "内核文件: $(ls -lh kernel.bin | awk '{print $5}') ($(date -r kernel.bin '+%Y-%m-%d %H:%M:%S'))"
echo "ISO文件: $(ls -lh ../os.iso | awk '{print $5}') ($(date -r ../os.iso '+%Y-%m-%d %H:%M:%S'))"
echo ""
echo "运行测试命令:"
echo "  qemu-system-i386 -cdrom ../os.iso -m 512M"
echo "  qemu-system-i386 -cdrom ../os.iso -m 512M -nographic -serial file:serial.log"
echo "========================================="
