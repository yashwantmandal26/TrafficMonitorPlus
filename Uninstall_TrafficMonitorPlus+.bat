@echo off
setlocal EnableDelayedExpansion
title Uninstall TrafficMonitorPlus+

:: Check for Admin
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Requesting Administrator privileges to uninstall...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

echo Stopping TrafficMonitorPlus+...
taskkill /F /IM TrafficMonitor.exe >nul 2>&1
taskkill /F /IM TrafficMonitorPlus+.exe >nul 2>&1
timeout /t 1 /nobreak >nul

echo Removing Auto-Start Scheduled Task...
schtasks /delete /tn "TrafficMonitorPlus+" /f >nul 2>&1

echo Removing Desktop & Start Menu Shortcuts...
del /F /Q "%Public%\Desktop\TrafficMonitorPlus+.lnk" >nul 2>&1
del /F /Q "%UserProfile%\Desktop\TrafficMonitorPlus+.lnk" >nul 2>&1
rd /S /Q "%ProgramData%\Microsoft\Windows\Start Menu\Programs\TrafficMonitorPlus+" >nul 2>&1

echo Removing Installation Directory...
rd /S /Q "C:\Program Files\TrafficMonitorPlus+" >nul 2>&1

echo.
echo =======================================================
echo   TrafficMonitorPlus+ has been completely uninstalled.
echo =======================================================
echo.
pause
