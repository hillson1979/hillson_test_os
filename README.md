
cd /zhwh_os/test && make clean && make
cd /zhwh_os && make clean && make && ./zh.sh

1:in qemu:
cd /zhwh_os &&  qemu-system-i386 -vga std -display sdl -device piix3-usb-uhci,id=usb -device usb-mouse,bus=usb.0 -drive format=raw,file=os.iso -d int,cpu_reset -D qemu.log -serial file:./output_net2.txt

2:in computer:
first make a grub boot usb device,second copy kernel.bin lvglanet.elf into usb boot directory,copy grub.cfg into usb grub directory,finally  boot os from usb device.
