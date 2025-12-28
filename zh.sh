mkdir -p iso/boot/grub
cp kernel.bin iso/boot/
# 支持多种用户程序模块
cp test/shell_demo.elf iso/boot/ 2>/dev/null || echo "Warning: shell_demo.elf not found"
cp test/test.elf iso/boot/ 2>/dev/null || echo "Warning: test.elf not found, continuing without it"
cp test/simple_test.elf iso/boot/ 2>/dev/null || echo "Warning: simple_test.elf not found, continuing without it"
# 创建grub配置文件，支持多个启动选项
echo 'set timeout=5' > iso/boot/grub/grub.cfg
echo 'set default=1' >> iso/boot/grub/grub.cfg
echo '' >> iso/boot/grub/grub.cfg
# 选项1: 使用 simple_test (默认，最简单)
echo 'menuentry "My OS - Simple Test" {' >> iso/boot/grub/grub.cfg
echo '  multiboot /boot/kernel.bin' >> iso/boot/grub/grub.cfg
echo '  module /boot/simple_test.elf simple_test' >> iso/boot/grub/grub.cfg
echo '  boot' >> iso/boot/grub/grub.cfg
echo '}' >> iso/boot/grub/grub.cfg
echo '' >> iso/boot/grub/grub.cfg
# 选项2: 使用 shell_demo
echo 'menuentry "My OS - Shell Demo" {' >> iso/boot/grub/grub.cfg
echo '  multiboot /boot/kernel.bin' >> iso/boot/grub/grub.cfg
echo '  module /boot/shell_demo.elf shell_demo' >> iso/boot/grub/grub.cfg
echo '  boot' >> iso/boot/grub/grub.cfg
echo '}' >> iso/boot/grub/grub.cfg
echo '' >> iso/boot/grub/grub.cfg
# 选项3: 使用原始 test.elf
echo 'menuentry "My OS - Test Program" {' >> iso/boot/grub/grub.cfg
echo '  multiboot /boot/kernel.bin' >> iso/boot/grub/grub.cfg
echo '  module /boot/test.elf user_program' >> iso/boot/grub/grub.cfg
echo '  boot' >> iso/boot/grub/grub.cfg
echo '}' >> iso/boot/grub/grub.cfg
# 生成ISO镜像
grub-mkrescue -o os.iso iso
