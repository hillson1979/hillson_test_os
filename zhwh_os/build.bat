@echo off
echo Building OS...

REM Check if gcc is available
where gcc >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: gcc not found in PATH
    echo Please install MinGW-w64 or add it to PATH
    exit /b 1
)

REM Clean
echo Cleaning...
del /Q *.o 2>nul
del /Q kernel.bin 2>nul

REM Generate offsets
echo Generating offsets...
gcc -m32 -c gen_asm_offsets.c -o gen_asm_offsets.o
gcc -m32 -o gen_asm_offsets gen_asm_offsets.o
.\gen_asm_offsets > task_offsets.s
echo Generated task_offsets.s

REM Compile C sources
echo Compiling C sources...
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c kernel.c -o kernel.o

gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c printf.c -o printf.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c vga.c -o vga.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c pci.c -o pci.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c kmalloc_early.c -o kmalloc_early.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c string.c -o string.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c highmem_mapping.c -o highmem_mapping.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c hardware_highmem.c -o hardware_highmem.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c madt_parser.c -o madt_parser.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c lapic.c -o lapic.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c ioapic.c -o ioapic.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c page.c -o page.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c acpi.c -o acpi.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c mp.c -o mp.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c segment.c -o segment.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c interrupt.c -o interrupt.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c mm.c -o mm.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c task.c -o task.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c sched.c -o sched.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c llist.c -o llist.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c signal.c -o signal.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c userboot.c -o userboot.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c syscall.c -o syscall.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c multiboot2.c -o multiboot2.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c pci_msi.c -o pci_msi.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c msi_test.c -o msi_test.o

REM Driver files
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c driver/keyboard.c -o driver_keyboard.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c driver/vbe.c -o driver_vbe.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c driver/uart.c -o driver_uart.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c driver/netdebug.c -o driver_netdebug.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c driver/usb_hcd.c -o driver_usb_hcd.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c driver/usb.c -o driver_usb.o
gcc -m32 -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone -mno-avx -mno-pclmul -mno-aes -fno-math-errno -fno-rounding-math -fno-signaling-nans -mgeneral-regs-only -I./include -c driver/usb_mouse.c -o driver_usb_mouse.o

echo Compile complete!
echo Build script would need more entries for full build...
echo Please use proper build environment with make
