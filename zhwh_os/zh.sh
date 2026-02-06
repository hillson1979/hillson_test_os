#!/bin/bash
cd "$(dirname "$0")" || exit 1
[ -d iso/boot/grub ] || mkdir -p iso/boot/grub
cp kernel.bin iso/boot/

# 只复制存在的 .elf 文件
[ -f test/direct_write_test.elf ] && cp test/direct_write_test.elf iso/boot/
[ -f test/sched_test5.elf ] && cp test/sched_test5.elf iso/boot/
[ -f test/shell_demo.elf ] && cp test/shell_demo.elf iso/boot/
[ -f test/simple_printf_test.elf ] && cp test/simple_printf_test.elf iso/boot/
[ -f test/simple_syscall_test.elf ] && cp test/simple_syscall_test.elf iso/boot/
[ -f test/fs_test.elf ] && cp test/fs_test.elf iso/boot/
[ -f test/net_test.elf ] && cp test/net_test.elf iso/boot/
[ -f test/net_shell.elf ] && cp test/net_shell.elf iso/boot/

cat > iso/boot/grub/grub.cfg << 'EOF'
set timeout=5
set default=4



menuentry "My OS - 5 Process Test" {
  multiboot2 /boot/kernel.bin
  module2 /boot/sched_test5.elf
  boot
}

menuentry "My OS - shell_demo Test" {
  multiboot2 /boot/kernel.bin
  module2 /boot/shell_demo.elf
  boot
}



menuentry "My OS - File System Test" {
  multiboot2 /boot/kernel.bin
  module2 /boot/fs_test.elf
  boot
}

menuentry "My OS - Network Test" {
  multiboot2 /boot/kernel.bin
  module2 /boot/net_test.elf
  boot
}

menuentry "My OS - Network Shell (Interactive)" {
  multiboot2 /boot/kernel.bin
  module2 /boot/net_shell.elf
  boot
}

EOF
grub-mkrescue -o os.iso iso
echo "ISO created: os.iso"