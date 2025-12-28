#!/bin/bash
# 使用 grub-mkrescue 创建可引导 ISO 镜像

cd /mnt/f/hillson_test_os/hillson_test_os

echo "准备 ISO 内容..."

# 创建临时目录结构
mkdir -p iso_temp/boot/grub

# 复制内核和模块
cp kernel.bin iso_temp/boot/
cp test/shell_demo.elf iso_temp/boot/
cp test/test.elf iso_temp/boot/

# 创建 GRUB 配置
cat > iso_temp/boot/grub/grub.cfg << 'EOF'
set timeout=5
set default=0

menuentry "My OS - Shell Demo" {
  multiboot /boot/kernel.bin
  module /boot/shell_demo.elf shell_demo
  boot
}

menuentry "My OS - Test Program" {
  multiboot /boot/kernel.bin
  module /boot/test.elf user_program
  boot
}
EOF

echo "正在使用 grub-mkrescue 创建 ISO 镜像..."
grub-mkrescue -o os.iso iso_temp 2>&1

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ ISO 镜像创建成功: os.iso"
    ls -lh os.iso
    echo ""
    echo "运行命令："
    echo "  qemu-system-i386 -cdrom os.iso -m 4096 -nographic"
else
    echo "✗ ISO 镜像创建失败"
    exit 1
fi

# 清理临时文件
rm -rf iso_temp
