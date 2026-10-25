@echo off
setlocal EnableDelayedExpansion
title TrafficMonitorPlus+ Setup

:: Check for Administrative Privileges and self-elevate
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Requesting Administrator privileges to install TrafficMonitorPlus+...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

echo =======================================================
echo          TrafficMonitorPlus+ Installation Setup
echo            Created by Yashwant Mandal
echo =======================================================
echo.

set "INSTALL_DIR=C:\Program Files\TrafficMonitorPlus+"

:: Kill running instances if any
taskkill /F /IM TrafficMonitor.exe >nul 2>&1
taskkill /F /IM TrafficMonitorPlus+.exe >nul 2>&1
timeout /t 1 /nobreak >nul

echo [1/5] Creating installation directory: "%INSTALL_DIR%"...
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"

echo [2/5] Copying application files and libraries...
set "SRC_DIR=%~dp0"
copy /Y "%SRC_DIR%TrafficMonitorPlus+.exe" "%INSTALL_DIR%\" >nul
copy /Y "%SRC_DIR%TrafficMonitor.exe" "%INSTALL_DIR%\" >nul 2>&1
copy /Y "%SRC_DIR%OpenHardwareMonitorApi.dll" "%INSTALL_DIR%\" >nul
copy /Y "%SRC_DIR%LibreHardwareMonitorLib.dll" "%INSTALL_DIR%\" >nul
copy /Y "%SRC_DIR%LibreHardwareMonitorLib.xml" "%INSTALL_DIR%\" >nul
copy /Y "%SRC_DIR%config.ini" "%INSTALL_DIR%\" >nul
copy /Y "%SRC_DIR%global_cfg.ini" "%INSTALL_DIR%\" >nul 2>&1
copy /Y "%SRC_DIR%Uninstall_TrafficMonitorPlus+.bat" "%INSTALL_DIR%\" >nul 2>&1

:: Copy directories
if exist "%SRC_DIR%language" (
    if not exist "%INSTALL_DIR%\language" mkdir "%INSTALL_DIR%\language"
    xcopy /E /I /Y "%SRC_DIR%language" "%INSTALL_DIR%\language" >nul
)
if exist "%SRC_DIR%skins" (
    if not exist "%INSTALL_DIR%\skins" mkdir "%INSTALL_DIR%\skins"
    xcopy /E /I /Y "%SRC_DIR%skins" "%INSTALL_DIR%\skins" >nul
)
if exist "%SRC_DIR%plugins" (
    if not exist "%INSTALL_DIR%\plugins" mkdir "%INSTALL_DIR%\plugins"
    xcopy /E /I /Y "%SRC_DIR%plugins" "%INSTALL_DIR%\plugins" >nul
)

echo [3/5] Creating Desktop and Start Menu Shortcuts...
powershell -NoProfile -Command "$ws = New-Object -ComObject WScript.Shell; $s = $ws.CreateShortcut([Environment]::GetFolderPath('CommonDesktopDirectory') + '\TrafficMonitorPlus+.lnk'); $s.TargetPath = '%INSTALL_DIR%\TrafficMonitorPlus+.exe'; $s.WorkingDirectory = '%INSTALL_DIR%'; $s.Description = 'TrafficMonitorPlus+ by Yashwant Mandal'; $s.Save()"

set "START_MENU_DIR=%ProgramData%\Microsoft\Windows\Start Menu\Programs\TrafficMonitorPlus+"
if not exist "%START_MENU_DIR%" mkdir "%START_MENU_DIR%"
powershell -NoProfile -Command "$ws = New-Object -ComObject WScript.Shell; $s = $ws.CreateShortcut('%START_MENU_DIR%\TrafficMonitorPlus+.lnk'); $s.TargetPath = '%INSTALL_DIR%\TrafficMonitorPlus+.exe'; $s.WorkingDirectory = '%INSTALL_DIR%'; $s.Description = 'TrafficMonitorPlus+ by Yashwant Mandal'; $s.Save()"

echo [4/5] Setting up Silent Auto-Start on Windows Boot (No UAC Prompts)...
schtasks /create /tn "TrafficMonitorPlus+" /tr "\"%INSTALL_DIR%\TrafficMonitorPlus+.exe\"" /sc onlogon /rl HIGHEST /f >nul 2>&1

echo [5/5] Launching TrafficMonitorPlus+...
start "" "%INSTALL_DIR%\TrafficMonitorPlus+.exe"

echo.
echo =======================================================
echo     Installation Completed Successfully!
echo     Look at your Taskbar (right side) to see telemetry.
echo =======================================================
echo.
pause
