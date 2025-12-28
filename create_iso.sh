#!/bin/bash
# 创建 ISO 镜像脚本

cd /mnt/f/hillson_test_os/hillson_test_os

echo "正在复制文件到 iso 目录..."
cp kernel.bin iso/boot/
cp test/shell_demo.elf iso/boot/
cp test/test.elf iso/boot/

echo "正在创建 ISO 镜像..."
genisoimage -R -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 \
    -boot-info-table -o os.iso iso 2>&1

if [ $? -eq 0 ]; then
    echo "ISO 镜像创建成功: os.iso"
    ls -lh os.iso
else
    echo "ISO 镜像创建失败"
    exit 1
fi
