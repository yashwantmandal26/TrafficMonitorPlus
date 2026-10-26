@echo off
title Package TrafficMonitorPlus+ Release
echo Packaging TrafficMonitorPlus+ for GitHub Release...
powershell -ExecutionPolicy Bypass -File "%~dp0Package_Release.ps1"
pause
