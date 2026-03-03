@echo off
REM Test keyboard input in LVGL and user process execution
REM This script builds the kernel and runs QEMU with LVGL GUI test

echo ======================================
echo Testing Keyboard Input in LVGL
echo ======================================
echo.

REM Check if kernel.bin exists, if not, try to build
if not exist kernel.bin (
    echo Kernel not found. Please build first with:
    echo   make (Linux/WSL)
    echo.
    goto :end
)

REM Check if lvglanet.elf exists
if not exist test\lvglanet.elf (
    echo Warning: test\lvglanet.elf not found
    echo Building user programs...
    cd test
    make
    cd ..
)

REM Create ISO directory structure
if not exist iso\boot\grub mkdir iso\boot\grub

REM Copy kernel
copy /Y kernel.bin iso\boot\ >nul

REM Copy LVGL test program
if exist test\lvglanet.elf copy /Y test\lvglanet.elf iso\boot\ >nul

REM Create GRUB config
(
echo set timeout=5
echo set default=0
echo.
echo insmod all_video
echo insmod gfxterm
echo insmod multiboot2
echo set gfxmode=1024x768x32
echo terminal_output gfxterm
echo.
echo menuentry "Test LVGL Keyboard Input" {
echo     multiboot2 /boot/kernel.bin
echo     module2 /boot/lvglanet.elf
echo     boot
echo }
) > iso\boot\grub\grub.cfg

echo.
echo ======================================
echo Starting QEMU with LVGL test...
echo ======================================
echo.
echo Instructions:
echo 1. The OS should boot and display LVGL interface
echo 2. Type on your keyboard and verify characters appear
echo 3. Check that user process (lvglanet.elf) runs correctly
echo 4. Press Ctrl+Alt+Q to exit QEMU (SDL mode)
echo.

REM Run QEMU with keyboard focus
qemu-system-i386 -cdrom os.iso -vga std -display sdl -serial file:test_keyboard_output.txt -no-reboot

echo.
echo ======================================
echo Test completed
echo ======================================
echo.
echo Output saved to: test_keyboard_output.txt
echo.

:end
pause
