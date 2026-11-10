cd /zhwh_os && make clean && make && ./zh.sh

1:in qemu:
cd /zhwh_os && qemu-system-i386 -m 2048 -cdrom os.iso  -boot d -vga std -display sdl -device nec-usb-xhci,id=xhci -serial file:output.txt

2:in computer:
first make a grub legacy boot usb device,second copy kernel.bin desktop.elf into usb boot directory,copy grub.cfg into usb grub directory,finally  boot os from usb device.
