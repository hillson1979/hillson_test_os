#!/bin/bash
# 手动创建可引导的 GRUB ISO 镜像

cd /mnt/f/hillson_test_os/hillson_test_os

echo "正在创建可引导 ISO..."

# 清理旧文件
rm -rf iso_boot
mkdir -p iso_boot/boot/grub

# 复制内核和模块
cp kernel.bin iso_boot/boot/
cp test/shell_demo.elf iso_boot/boot/
cp test/test.elf iso_boot/boot/

# 创建 GRUB 配置
cat > iso_boot/boot/grub/grub.cfg << 'EOF'
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

# 检查是否有 GRUB 引导文件
if [ -f /usr/lib/grub/i386-pc/boot.img ]; then
    echo "使用 GRUB legacy 引导..."

    # 创建 core.img
    mkdir -p iso_boot/boot/grub/i386-pc

    # 使用 grub-mkimage 创建 core.img
    grub-mkimage -O i386-pc -p "(hd0)/boot/grub" -o iso_boot/boot/grub/i386-pc/core.img \
        biosdisk part_msdos ext2 multiboot normal sh ls echo cat test \
        search configfile help reboot halt

    # 复制 boot.img
    cp /usr/lib/grub/i386-pc/boot.img iso_boot/boot/grub/i386-pc/

    # 使用 genisoimage 创建混合引导 ISO
    genisoimage -R -b boot/grub/i386-pc/boot.img \
        -no-emul-boot -boot-load-size 4 \
        -boot-info-table -o os.iso iso_boot

    if [ $? -eq 0 ]; then
        echo "✓ ISO 创建成功"
        ls -lh os.iso
    else
        echo "✗ ISO 创建失败"
    fi
else
    echo "错误: GRUB 引导文件未找到"
    echo "请安装: sudo apt-get install grub-pc-bin"
    exit 1
fi

rm -rf iso_boot
