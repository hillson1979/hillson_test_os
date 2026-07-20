@echo off
REM ========================================================
REM Text Editor Build Script for Windows
REM zhwh_os - 轻量级文本编辑器
REM ========================================================

echo Building Text Editor...

REM Change to project directory
cd /d "%~dp0"

REM First build the test program (which includes LVGL)
echo.
echo Building test (to get LVGL objects)...
cd ..\test
make

echo.
echo ========================================================
echo Text Editor Build Complete!
echo.
echo To integrate text_editor.elf into the OS:
echo   1. Copy text_editor.elf to iso/boot/
echo   2. Run build.bat in zhwh_os directory
echo ========================================================
cd ..\text_editor

pause
