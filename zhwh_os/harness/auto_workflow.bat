@echo off
REM OS Development Automated Workflow Script (Windows)
REM This is a wrapper for the bash script
REM
REM Usage: auto_workflow.bat [--feature FEATURE_ID] [--skip-build] [--skip-test]

setlocal

echo ========================================
echo OS Development Automated Workflow
echo ========================================
echo.

REM Check if WSL is available
where wsl.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: WSL not found. Please install WSL or run the script directly in WSL.
    exit /b 1
)

REM Change to project directory
cd /d "%~dp0..\.."

REM Run the workflow script in WSL
echo Running workflow in WSL...
echo.

wsl bash -c "cd /mnt/f/hillson_test_os/zhwh_os && ./harness/auto_workflow.sh %*"

echo.
echo ========================================
echo Workflow Complete
echo ========================================
echo.

endlocal
