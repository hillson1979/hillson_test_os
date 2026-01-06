#!/bin/bash
# ISO 创建脚本

cd "$(dirname "$0")"

mkdir -p iso/boot/grub
cp kernel.bin iso/boot/
# 复制 Multiboot 2 测试内核（如果存在）
# 支持多种用户程序模块
cp test/shell_demo.elf iso/boot/ 2>/dev/null || echo "Warning: shell_demo.elf not found"

# 创建grub配置文件，支持多个启动选项
echo 'set timeout=10' > iso/boot/grub/grub.cfg
# 显式启用图形模式，让 GRUB 设置 LFB
echo 'set gfxmode=800x600x32' >> iso/boot/grub/grub.cfg
echo 'set default=0' >> iso/boot/grub/grub.cfg  # 默认：shell_demo (有用户程序)
echo '' >> iso/boot/grub/grub.cfg
# 选项0: 无模块启动 (测试 framebuffer，使用 VBE 模式 0x118)
echo 'menuentry "My OS - No Module (VBE 0x118)" {' >> iso/boot/grub/grub.cfg
echo '  multiboot /boot/kernel.bin vga=0x118' >> iso/boot/grub/grub.cfg
echo '  boot' >> iso/boot/grub/grub.cfg
echo '}' >> iso/boot/grub/grub.cfg
echo '' >> iso/boot/grub/grub.cfg

# 选项4: 使用 shell_demo（添加 VBE 参数测试）
echo 'menuentry "My OS - Shell Demo" {' >> iso/boot/grub/grub.cfg
echo '  multiboot /boot/kernel.bin vga=0x118' >> iso/boot/grub/grub.cfg
echo '  module /boot/shell_demo.elf shell_demo' >> iso/boot/grub/grub.cfg
echo '  boot' >> iso/boot/grub/grub.cfg
echo '}' >> iso/boot/grub/grub.cfg
echo '' >> iso/boot/grub/grub.cfg


# 生成ISO镜像
grub-mkrescue -o os.iso iso
