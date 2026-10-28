@echo off
setlocal EnableDelayedExpansion
title TrafficMonitorPlus+ Installer

:: Try to elevate to Administrator if not already admin
net session >nul 2>&1
if %errorlevel% neq 0 (
    powershell -NoProfile -Command "Start-Process '%~f0' -Verb RunAs" 2>nul
    if !errorlevel! equ 0 (
        exit /b
    )
)

set "SCRIPT_DIR=%~dp0"
set "LOCAL_PS1=%SCRIPT_DIR%install.ps1"

if exist "%LOCAL_PS1%" (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%LOCAL_PS1%"
) else (
    echo [*] Fetching and running installer from GitHub...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "irm https://raw.githubusercontent.com/yashwantmandal26/TrafficMonitorPlus/master/install.ps1 | iex"
)
