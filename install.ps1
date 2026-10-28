# TrafficMonitorPlus+ - Windows 11 Automated Installer
# Supports 1-line Web install (irm ... | iex) or local execution.
# Created by Yashwant Mandal

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

# Ensure TLS 1.2 and 1.3
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 -bor [Net.SecurityProtocolType]::Tls13

# Check for Administrator Privileges
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
$installDir = "C:\Program Files\TrafficMonitorPlus+"

if (-not $isAdmin) {
    $elevatedSuccessfully = $false
    try {
        $scriptUrl = "https://raw.githubusercontent.com/yashwantmandal26/TrafficMonitorPlus/master/install.ps1"
        $tempScript = Join-Path $env:TEMP "install_TrafficMonitorPlus.ps1"

        if ($MyInvocation.MyCommand.Path -and (Test-Path $MyInvocation.MyCommand.Path)) {
            $proc = Start-Process powershell -Verb RunAs -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$($MyInvocation.MyCommand.Path)`"" -PassThru -ErrorAction Stop
            $elevatedSuccessfully = $true
        } else {
            Invoke-WebRequest -Uri $scriptUrl -OutFile $tempScript -UseBasicParsing
            $proc = Start-Process powershell -Verb RunAs -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$tempScript`"" -PassThru -ErrorAction Stop
            $elevatedSuccessfully = $true
        }
    } catch {
        $elevatedSuccessfully = $false
    }

    if ($elevatedSuccessfully) {
        exit 0
    }

    Write-Host "[*] Admin elevation not available or skipped. Installing to Local AppData..." -ForegroundColor Yellow
    $installDir = Join-Path $env:LOCALAPPDATA "TrafficMonitorPlus+"
}

Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host "       TrafficMonitorPlus+ Windows 11 Setup            " -ForegroundColor Cyan
Write-Host "            Created by Yashwant Mandal                 " -ForegroundColor Cyan
Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host ""

$cleanupDirs = @()

# 1. Close existing running instances
Write-Host "[1/7] Stopping any running instances..." -ForegroundColor Yellow
Get-Process -Name "TrafficMonitorPlus+","TrafficMonitor" -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 800

# 2. Check and ensure Visual C++ Runtime prerequisites
Write-Host "[2/7] Checking Visual C++ Runtime prerequisites..." -ForegroundColor Yellow
$sysMfc = Join-Path $env:SystemRoot "System32\mfc140u.dll"
if (-not (Test-Path $sysMfc)) {
    Write-Host "  -> Microsoft Visual C++ MFC Runtime is missing on this PC." -ForegroundColor Yellow
    Write-Host "  -> Downloading official Microsoft Visual C++ Redistributable (x64)..." -ForegroundColor Yellow
    $vcRedistUrl = "https://aka.ms/vs/17/release/vc_redist.x64.exe"
    $vcTemp = Join-Path $env:TEMP "vc_redist.x64.exe"
    try {
        Invoke-WebRequest -Uri $vcRedistUrl -OutFile $vcTemp -UseBasicParsing
        Write-Host "  -> Installing Visual C++ Redistributable silently..." -ForegroundColor Yellow
        $vcProc = Start-Process -FilePath $vcTemp -ArgumentList "/install /quiet /norestart" -Wait -PassThru -ErrorAction Stop
        Remove-Item $vcTemp -Force -ErrorAction SilentlyContinue
        Write-Host "  -> Visual C++ Runtime installed successfully!" -ForegroundColor Green
    } catch {
        Write-Host "  -> Note: Bundled local runtime DLLs will be installed directly into app folder." -ForegroundColor Gray
    }
} else {
    Write-Host "  -> Visual C++ Runtime is present." -ForegroundColor Green
}

# 3. Clean up legacy vulnerable services
if ($isAdmin) {
    Write-Host "[3/7] Cleaning up legacy driver services (preventing Defender alerts)..." -ForegroundColor Yellow
    try { & sc.exe stop R0TrafficMonitorPlus+ 2>&1 | Out-Null } catch {}
    try { & sc.exe delete R0TrafficMonitorPlus+ 2>&1 | Out-Null } catch {}
    try { & sc.exe stop R0TrafficMonitor 2>&1 | Out-Null } catch {}
    try { & sc.exe delete R0TrafficMonitor 2>&1 | Out-Null } catch {}
} else {
    Write-Host "[3/7] Skipping service cleanup (user-level install)..." -ForegroundColor Yellow
}
try { & schtasks.exe /delete /tn "\TrafficMonitor\Autorun for $env:USERNAME" /f 2>&1 | Out-Null } catch {}

# 4. Locate or Download Source Files
Write-Host "[4/7] Locating package binaries..." -ForegroundColor Yellow
$srcDir = $null

if ($PSScriptRoot -and (Test-Path (Join-Path $PSScriptRoot "TrafficMonitorPlus+.exe"))) {
    $srcDir = $PSScriptRoot
    Write-Host "  -> Using local repository files at: $srcDir" -ForegroundColor Green
} elseif ($PSScriptRoot -and (Test-Path (Join-Path (Split-Path -Parent $PSScriptRoot) "TrafficMonitorPlus+.exe"))) {
    $srcDir = Split-Path -Parent $PSScriptRoot
    Write-Host "  -> Using parent folder files at: $srcDir" -ForegroundColor Green
} else {
    Write-Host "  -> Downloading latest package from GitHub..." -ForegroundColor Yellow
    $tempZip = Join-Path $env:TEMP "TrafficMonitorPlus_Latest.zip"
    $tempExtract = Join-Path $env:TEMP "TrafficMonitorPlus_Extracted"
    $cleanupDirs += $tempExtract
    $cleanupDirs += $tempZip

    if (Test-Path $tempExtract) {
        Remove-Item -Path $tempExtract -Recurse -Force -ErrorAction SilentlyContinue
    }
    if (Test-Path $tempZip) {
        Remove-Item -Path $tempZip -Force -ErrorAction SilentlyContinue
    }

    $zipUrl = "https://github.com/yashwantmandal26/TrafficMonitorPlus/archive/refs/heads/master.zip"
    Write-Host "  -> Downloading $zipUrl ..." -ForegroundColor Gray
    Invoke-WebRequest -Uri $zipUrl -OutFile $tempZip -UseBasicParsing

    Write-Host "  -> Extracting package..." -ForegroundColor Gray
    Expand-Archive -Path $tempZip -DestinationPath $tempExtract -Force

    $found = Get-ChildItem -Path $tempExtract -Recurse -Filter "TrafficMonitorPlus+.exe" | Select-Object -First 1
    if (-not $found) {
        throw "Failed to find TrafficMonitorPlus+.exe in the downloaded package."
    }
    $srcDir = $found.DirectoryName
    Write-Host "  -> Package extracted successfully." -ForegroundColor Green
}

# 5. Install files to destination directory
Write-Host "[5/7] Installing TrafficMonitorPlus+ to '$installDir'..." -ForegroundColor Yellow
if (-not (Test-Path $installDir)) {
    New-Item -ItemType Directory -Path $installDir -Force | Out-Null
}

$coreFiles = @(
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
    "Uninstall_TrafficMonitorPlus+.bat"
)

foreach ($f in $coreFiles) {
    $fPath = Join-Path $srcDir $f
    if (Test-Path $fPath) {
        Copy-Item -Path $fPath -Destination $installDir -Force
    }
}

# Preserve user's config if present; otherwise copy default config
$targetConfig = Join-Path $installDir "config.ini"
$srcConfig = Join-Path $srcDir "config.ini"
if ((-not (Test-Path $targetConfig)) -and (Test-Path $srcConfig)) {
    Copy-Item -Path $srcConfig -Destination $targetConfig -Force
}

# Ensure AppData/Roaming configuration is also seeded with taskbar-only defaults
$appDataConfigDir = Join-Path $env:APPDATA "TrafficMonitor"
if (-not (Test-Path $appDataConfigDir)) {
    New-Item -ItemType Directory -Path $appDataConfigDir -Force | Out-Null
}
$appDataConfigFile = Join-Path $appDataConfigDir "config.ini"
if ((-not (Test-Path $appDataConfigFile)) -and (Test-Path $srcConfig)) {
    Copy-Item -Path $srcConfig -Destination $appDataConfigFile -Force
}

$srcGlobalCfg = Join-Path $srcDir "global_cfg.ini"
if (Test-Path $srcGlobalCfg) {
    Copy-Item -Path $srcGlobalCfg -Destination $installDir -Force
    Copy-Item -Path $srcGlobalCfg -Destination $appDataConfigDir -Force
}

# Copy subdirectories (skins, language, plugins)
$subDirs = @("skins", "language", "plugins")
foreach ($d in $subDirs) {
    $dPath = Join-Path $srcDir $d
    if (Test-Path $dPath) {
        Copy-Item -Path $dPath -Destination (Join-Path $installDir $d) -Recurse -Force
    }
}

# 6. Create Shortcuts and Startup Entry
Write-Host "[6/7] Creating Desktop and Start Menu shortcuts..." -ForegroundColor Yellow
$wsh = New-Object -ComObject WScript.Shell

# Desktop shortcut
$desktopDir = if ($isAdmin) { [Environment]::GetFolderPath('CommonDesktopDirectory') } else { [Environment]::GetFolderPath('Desktop') }
if (-not (Test-Path $desktopDir)) {
    $desktopDir = [Environment]::GetFolderPath('Desktop')
}
$desktopLnk = Join-Path $desktopDir "TrafficMonitorPlus+.lnk"
$sc = $wsh.CreateShortcut($desktopLnk)
$sc.TargetPath = Join-Path $installDir "TrafficMonitorPlus+.exe"
$sc.WorkingDirectory = $installDir
$sc.Description = "TrafficMonitorPlus+ for Windows 11"
$sc.Save()

# Start Menu shortcut
$startMenuDir = if ($isAdmin) {
    "$env:ProgramData\Microsoft\Windows\Start Menu\Programs\TrafficMonitorPlus+"
} else {
    "$env:APPDATA\Microsoft\Windows\Start Menu\Programs\TrafficMonitorPlus+"
}
if (-not (Test-Path $startMenuDir)) {
    New-Item -ItemType Directory -Path $startMenuDir -Force | Out-Null
}
$startMenuLnk = Join-Path $startMenuDir "TrafficMonitorPlus+.lnk"
$sc2 = $wsh.CreateShortcut($startMenuLnk)
$sc2.TargetPath = Join-Path $installDir "TrafficMonitorPlus+.exe"
$sc2.WorkingDirectory = $installDir
$sc2.Description = "TrafficMonitorPlus+ for Windows 11"
$sc2.Save()

# Auto-start configuration
Write-Host "  -> Registering Auto-Start on Windows Logon..." -ForegroundColor Gray
if ($isAdmin) {
    try { & schtasks.exe /create /tn "TrafficMonitorPlus+" /tr "`"$installDir\TrafficMonitorPlus+.exe`"" /sc onlogon /rl HIGHEST /f 2>&1 | Out-Null } catch {}
} else {
    $regPath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"
    Set-ItemProperty -Path $regPath -Name "TrafficMonitorPlus+" -Value "`"$installDir\TrafficMonitorPlus+.exe`"" -Force
}

# 7. Launch Application
Write-Host "[7/7] Launching TrafficMonitorPlus+..." -ForegroundColor Green
$targetExe = Join-Path $installDir "TrafficMonitorPlus+.exe"
$launched = $false

# Method 1: If scheduled task exists, run via schtasks (runs with highest privileges silently)
if ($isAdmin) {
    try {
        & schtasks.exe /run /tn "TrafficMonitorPlus+" 2>&1 | Out-Null
        $launched = $true
    } catch {}
}

# Method 2: Direct launch fallback
if (-not $launched) {
    try {
        $wshell = New-Object -ComObject WScript.Shell
        $wshell.CurrentDirectory = $installDir
        $wshell.Run("cmd /c start `"`" `"$targetExe`"", 0, $false) | Out-Null
        $launched = $true
    } catch {}
}

# Cleanup temp files
foreach ($item in $cleanupDirs) {
    if (Test-Path $item) {
        Remove-Item -Path $item -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Write-Host ""
Write-Host "=======================================================" -ForegroundColor Green
Write-Host "   TrafficMonitorPlus+ Installed & Running!           " -ForegroundColor Green
Write-Host "=======================================================" -ForegroundColor Green
Write-Host "   - Status: Active in Windows 11 Taskbar             " -ForegroundColor White
Write-Host "   - Auto-Start: Enabled on Logon                     " -ForegroundColor White
Write-Host "   - Install Location: $installDir                    " -ForegroundColor White
Write-Host "   - Shortcuts: Created on Desktop & Start Menu       " -ForegroundColor White
Write-Host "=======================================================" -ForegroundColor Green
Write-Host ""
