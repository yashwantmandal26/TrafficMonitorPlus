# PowerShell script to package TrafficMonitorPlus+ for GitHub Release
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host "  Packaging TrafficMonitorPlus+ for GitHub Release   " -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan

$distDir = Join-Path $root "dist"
$pkgDir = Join-Path $distDir "TrafficMonitorPlus+_v1.86_x64"
$zipFile = Join-Path $distDir "TrafficMonitorPlus+_v1.86_x64.zip"

if (Test-Path $distDir) {
    Remove-Item $distDir -Recurse -Force
}
New-Item -ItemType Directory -Path $pkgDir -Force | Out-Null

Write-Host "`n[*] Copying production runtime binaries and libraries..." -ForegroundColor Yellow
$filesToCopy = @(
    "TrafficMonitorPlus+.exe",
    "TrafficMonitor.exe",
    "OpenHardwareMonitorApi.dll",
    "LibreHardwareMonitorLib.dll",
    "LibreHardwareMonitorLib.xml",
    "mfc140u.dll",
    "mfcm140u.dll",
    "msvcp140.dll",
    "vcruntime140.dll",
    "vcruntime140_1.dll",
    "config.ini",
    "global_cfg.ini",
    "install.bat",
    "install.ps1",
    "Install_TrafficMonitorPlus+.bat",
    "Uninstall_TrafficMonitorPlus+.bat",
    "README.md",
    "README_en-us.md",
    "LICENSE"
)

foreach ($file in $filesToCopy) {
    $src = Join-Path $root $file
    if (Test-Path $src) {
        Copy-Item -Path $src -Destination $pkgDir -Force
        Write-Host "  + $file" -ForegroundColor Green
    }
}

Write-Host "`n[*] Copying asset directories (language, skins, plugins)..." -ForegroundColor Yellow
$dirsToCopy = @("language", "skins", "plugins")
foreach ($dir in $dirsToCopy) {
    $src = Join-Path $root $dir
    if (Test-Path $src) {
        Copy-Item -Path $src -Destination (Join-Path $pkgDir $dir) -Recurse -Force
        Write-Host "  + $dir\" -ForegroundColor Green
    }
}

Write-Host "`n[*] Creating release ZIP archive: TrafficMonitorPlus+_v1.86_x64.zip..." -ForegroundColor Yellow
Compress-Archive -Path "$pkgDir\*" -DestinationPath $zipFile -Force

$zipSize = (Get-Item $zipFile).Length / 1MB
Write-Host "`n=====================================================" -ForegroundColor Green
Write-Host "  SUCCESS! Release package generated successfully!   " -ForegroundColor Green
Write-Host "  Location: $zipFile" -ForegroundColor Cyan
Write-Host "  Size: $([math]::Round($zipSize, 2)) MB" -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Green
