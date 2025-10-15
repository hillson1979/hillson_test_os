mkdir -p iso/boot/grub
cp kernel.bin iso/boot/
cp ../test/test.elf iso/boot/
# 创建grub配置文件，指定内核启动命令
echo 'set timeout=0' > iso/boot/grub/grub.cfg
echo 'set default=0' >> iso/boot/grub/grub.cfg
echo 'menuentry "My OS" {' >> iso/boot/grub/grub.cfg
echo '  multiboot /boot/kernel.bin' >> iso/boot/grub/grub.cfg
echo '  module /boot/test.elf user_program' >> iso/boot/grub/grub.cfg
echo '  boot' >> iso/boot/grub/grub.cfg
echo '}' >> iso/boot/grub/grub.cfg
# 生成ISO镜像
#cd iso/boot 
grub-mkrescue -o kernel.iso iso
