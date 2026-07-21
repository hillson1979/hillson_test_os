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
[ -f qt/cpp_test.elf ] && cp qt/cpp_test.elf iso/boot/
[ -f qt/sigslot_test.elf ] && cp qt/sigslot_test.elf iso/boot/
[ -f qt/gui_test.elf ] && cp qt/gui_test.elf iso/boot/
[ -f qt/editor_test.elf ] && cp qt/editor_test.elf iso/boot/
[ -f text_editor/text_editor.elf ] && cp -f text_editor/text_editor.elf iso/boot/editor_test.elf
[ -f qt/fb_test.elf ] && cp qt/fb_test.elf iso/boot/
[ -f qt/qt_embedded_test.elf ] && cp qt/qt_embedded_test.elf iso/boot/
[ -f hillsonOs_desktop/desktop.elf ] && cp hillsonOs_desktop/desktop.elf iso/boot/
[ -f test/syscall_test.elf ] && cp test/syscall_test.elf iso/boot/
[ -f test/simple_keyboard.elf ] && cp test/simple_keyboard.elf iso/boot/
[ -f test/text_test.elf ] && cp test/text_test.elf iso/boot/
[ -f java/jvm.elf ] && cp java/jvm.elf iso/boot/

cat > iso/boot/grub/grub.cfg << 'EOF'
set timeout=5
set default=0

insmod vbe
insmod multiboot2
set gfxmode=1024x768x32
terminal_output gfxterm

menuentry "HillsonOS Desktop" {
    multiboot2 /boot/kernel.bin
    module2 /boot/desktop.elf
    module2 /boot/jvm.elf
    boot
}

menuentry "My OS - Text Mode Test" {
    multiboot2 /boot/kernel.bin
    module2 /boot/text_test.elf
    boot
}

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

menuentry "My OS - Simple Keyboard Test" {
    multiboot2 /boot/kernel.bin
    module2 /boot/simple_keyboard.elf
    boot
}

menuentry "My OS - Framebuffer Raw Test" {
    multiboot2 /boot/kernel.bin
    module2 /boot/fb_test.elf
    boot
}

menuentry "My OS - Text Editor (Interactive)" {
    multiboot2 /boot/kernel.bin
    module2 /boot/editor_test.elf
    boot
}

menuentry "My OS - GUI Framebuffer Test" {
    multiboot2 /boot/kernel.bin
    module2 /boot/gui_test.elf
    boot
}

menuentry "My OS - C++ Signal/Slot Test" {
    multiboot2 /boot/kernel.bin
    module2 /boot/sigslot_test.elf
    boot
}

menuentry "My OS - C++ Runtime Test" {
    multiboot2 /boot/kernel.bin
    module2 /boot/cpp_test.elf
    boot
}

menuentry "Qt/Embedded 3.3.8b Port Test" {
    multiboot2 /boot/kernel.bin
    module2 /boot/qt_embedded_test.elf
    boot
}

menuentry "My OS - System Call Test" {
    multiboot2 /boot/kernel.bin
    module2 /boot/syscall_test.elf
    boot
}

menuentry "Java VM - HelloWorld" {
    multiboot2 /boot/kernel.bin
    module2 /boot/jvm.elf HelloWorld
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
