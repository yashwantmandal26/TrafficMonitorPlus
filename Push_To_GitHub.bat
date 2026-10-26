@echo off
title Push TrafficMonitorPlus+ to GitHub
echo =======================================================
echo     Push TrafficMonitorPlus+ to your GitHub Account
echo =======================================================
echo.
echo Step 1: Go to https://github.com/new in your browser.
echo Step 2: Create a repository named "TrafficMonitorPlus" (Public, leave README unchecked).
echo Step 3: Copy your repository URL (e.g. https://github.com/YourUsername/TrafficMonitorPlus.git)
echo.
set /p REPO_URL="Paste your GitHub Repository URL here: "

if "%REPO_URL%"=="" (
    echo Error: No URL provided.
    pause
    exit /b
)

echo.
echo [*] Updating git remote origin to: %REPO_URL%...
git remote set-url origin %REPO_URL%

echo.
echo [*] Pushing all 26 custom commits and assets to GitHub...
git push -u origin master

if %errorlevel% equ 0 (
    echo.
    echo =======================================================
    echo   SUCCESS! TrafficMonitorPlus+ is now live on GitHub!
    echo.
    echo   Next: Go to your repo -> Releases -> Create Release
    echo   Attach: dist\TrafficMonitorPlus+_v1.86_x64.zip
    echo =======================================================
) else (
    echo.
    echo If authentication failed, GitHub may prompt you to sign in via browser.
)

echo.
pause
