@echo off
REM ========================================================
REM QEMU 启动脚本 - 带 UDP 端口转发
REM ========================================================
echo.
echo ========================================
echo   启动 QEMU - 视频播放器测试
echo ========================================
echo.
echo   主机 UDP 1234 -^> 虚拟机 UDP 1234
echo.
echo   ffmpeg 推流地址: udp://127.0.0.1:1234
echo.
echo ========================================
echo.

cd /d "%~dp0"

REM 检查 ISO 文件是否存在
if not exist "os.iso" (
    echo 错误: 找不到 os.iso 文件！
    echo 请先运行 zh.sh 构建 ISO
    pause
    exit /b 1
)

REM 启动 QEMU
qemu-system-i386 -cdrom os.iso -vga std -display sdl ^
  -netdev user,id=net0,hostfwd=udp::1234-:1234 ^
  -device e1000,netdev=net0 ^
  -device piix3-usb-uhci,id=usb -device usb-tablet,bus=usb.0 ^
  -serial file:output.txt

if errorlevel 1 (
    echo.
    echo QEMU 启动失败！
    echo 请确保 qemu-system-i386 在 PATH 中
    pause
)
