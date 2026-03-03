#!/bin/bash
cd "$(dirname "$0")" || exit 1
[ -d iso/boot/grub ] || mkdir -p iso/boot/grub
cp kernel.bin iso/boot/

# 只复制存在的 .elf 文件
[ -f test/fs_test.elf ] && cp test/fs_test.elf iso/boot/
[ -f test/net_test.elf ] && cp test/net_test.elf iso/boot/
[ -f test/net_shell.elf ] && cp test/net_shell.elf iso/boot/
[ -f test/gui_shell.elf ] && cp test/gui_shell.elf iso/boot/

[ -f test/lvglanet.elf ] && cp test/lvglanet.elf iso/boot/

cat > iso/boot/grub/grub.cfg << 'EOF'
set timeout=5
set default=4

# 加载视频模块并设置图形模式
insmod all_video
insmod gfxterm
insmod multiboot2
set gfxmode=1024x768x32
set gfxpayload=keep
terminal_output gfxterm
videoinfo


menuentry "My OS - File System Test" {
    multiboot2 /boot/kernel.bin
    module2 /boot/fs_test.elf
    boot
}

menuentry "My OS - Network Shell (Interactive)" {
    multiboot2 /boot/kernel.bin
    module2 /boot/net_shell.elf
    boot
}

menuentry "My OS - GUI Shell (Graphics)" {
    multiboot2 /boot/kernel.bin
    module2 /boot/gui_shell.elf
    boot
}

menuentry "My OS - LVGL Graphics Test" {
    multiboot2 /boot/kernel.bin
    module2 /boot/lvgl_test.elf
    boot
}

menuentry "My OS - LVGL + Network Test (Video Player)" {
    multiboot2 /boot/kernel.bin
    module2 /boot/lvglanet.elf
    boot
}

EOF
grub-mkrescue -o os.iso iso
echo "ISO created: os.iso"

echo ""
echo "============================================"
echo "To run with QEMU, use:"
echo "  qemu-system-i386 -cdrom os.iso -vga std -display sdl"
echo ""
echo "With USB mouse support:"
echo "  qemu-system-i386 -cdrom os.iso -vga std -display sdl -device piix3-usb-uhci,id=usb -device usb-mouse,bus=usb.0"
echo ""
echo "With serial output:"
echo "  qemu-system-i386 -cdrom os.iso -vga std -display sdl -serial file:./output.txt"
echo "============================================"
echo ""
