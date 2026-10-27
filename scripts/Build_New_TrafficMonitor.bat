@echo off
:: Self-elevate to Administrator
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Requesting Administrator privileges to compile TrafficMonitorPlus+...
    powershell -Command "Start-Process cmd -ArgumentList '/c \"\"%~f0\"\"' -Verb RunAs"
    exit /b
)

echo ============================================================
echo   TrafficMonitorPlus+ 100%% English & Dynamic Color Auto-Builder
echo ============================================================
echo.

:: 1. Check if msbuild exists
where msbuild >nul 2>&1
if %errorlevel% neq 0 (
    echo MSBuild not found. Installing Visual Studio C++ Build Tools with MFC...
    echo (This is a one-time automated download from Microsoft)
    echo Please wait...
    winget install --id Microsoft.VisualStudio.2022.BuildTools --accept-package-agreements --accept-source-agreements --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.ATLMFC --includeRecommended"
)

:: Find MSBuild path
for /f "usebackq tokens=*" %%i in (`powershell -Command "& '${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe' -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe"`) do (
    set MSBUILD_PATH=%%i
)

if not exist "%MSBUILD_PATH%" (
    echo Error: MSBuild was not found after installation.
    pause
    exit /b
)

echo Found MSBuild at: "%MSBUILD_PATH%"
echo.
echo Compiling TrafficMonitorPlus+ (Release x64)...
cd /d "%~dp0.."
"%MSBUILD_PATH%" TrafficMonitor\TrafficMonitor.vcxproj /p:Configuration="Release" /p:Platform=x64 /m

if %errorlevel% neq 0 (
    echo Compilation failed! Please check output above.
    pause
    exit /b
)

echo.
echo Stopping running TrafficMonitorPlus+...
taskkill /F /IM TrafficMonitorPlus+.exe >nul 2>&1
taskkill /F /IM TrafficMonitor.exe >nul 2>&1
timeout /t 1 /nobreak >nul

echo Updating repository and C:\Program Files\TrafficMonitorPlus+ with new binary...
if exist "TrafficMonitor\Bin\x64\Release\TrafficMonitor.exe" (`r`n    copy /Y "TrafficMonitor\Bin\x64\Release\TrafficMonitor.exe" "TrafficMonitorPlus+.exe" >nul`r`n    copy /Y "TrafficMonitor\Bin\x64\Release\TrafficMonitor.exe" "TrafficMonitor.exe" >nul`r`n    copy /Y "TrafficMonitor\Bin\x64\Release\TrafficMonitor.exe" "dist\TrafficMonitorPlus+_v1.86_x64\TrafficMonitorPlus+.exe" >nul 2>&1`r`n    copy /Y "TrafficMonitor\Bin\x64\Release\TrafficMonitor.exe" "dist\TrafficMonitorPlus+_v1.86_x64\TrafficMonitor.exe" >nul 2>&1`r`n    copy /Y "TrafficMonitor\Bin\x64\Release\TrafficMonitor.exe" "C:\Program Files\TrafficMonitorPlus+\TrafficMonitorPlus+.exe" >nul 2>&1`r`n    copy /Y "TrafficMonitor\Bin\x64\Release\TrafficMonitor.exe" "C:\Program Files\TrafficMonitorPlus+\TrafficMonitor.exe" >nul 2>&1`r`n    echo Successfully installed new TrafficMonitorPlus+.exe!`r`n) else if exist "Bin\x64\Release\TrafficMonitor.exe" (
    copy /Y "Bin\x64\Release\TrafficMonitor.exe" "TrafficMonitorPlus+.exe" >nul
    copy /Y "Bin\x64\Release\TrafficMonitor.exe" "TrafficMonitor.exe" >nul
    copy /Y "Bin\x64\Release\TrafficMonitor.exe" "C:\Program Files\TrafficMonitorPlus+\TrafficMonitorPlus+.exe" >nul 2>&1
    copy /Y "Bin\x64\Release\TrafficMonitor.exe" "C:\Program Files\TrafficMonitorPlus+\TrafficMonitor.exe" >nul 2>&1
    if exist "Bin\x64\Release\OpenHardwareMonitorApi.dll" (
        copy /Y "Bin\x64\Release\OpenHardwareMonitorApi.dll" "C:\Program Files\TrafficMonitorPlus+\OpenHardwareMonitorApi.dll" >nul 2>&1
    )
    echo Successfully installed new TrafficMonitorPlus+.exe!
) else if exist "x64\Release\TrafficMonitor.exe" (
    copy /Y "x64\Release\TrafficMonitor.exe" "TrafficMonitorPlus+.exe" >nul
    copy /Y "x64\Release\TrafficMonitor.exe" "TrafficMonitor.exe" >nul
    copy /Y "x64\Release\TrafficMonitor.exe" "C:\Program Files\TrafficMonitorPlus+\TrafficMonitorPlus+.exe" >nul 2>&1
    copy /Y "x64\Release\TrafficMonitor.exe" "C:\Program Files\TrafficMonitorPlus+\TrafficMonitor.exe" >nul 2>&1
    echo Successfully installed new TrafficMonitorPlus+.exe!
)

echo Starting updated TrafficMonitorPlus+...
start "" "C:\Program Files\TrafficMonitorPlus+\TrafficMonitorPlus+.exe"

echo.
echo ============================================================
echo   SUCCESS! TrafficMonitorPlus+ is now running!
echo ============================================================
timeout /t 5
