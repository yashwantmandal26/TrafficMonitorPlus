$ErrorActionPreference = 'Stop'
$source = Split-Path -Parent $PSScriptRoot
if (!(Test-Path -LiteralPath "$source/TrafficMonitorPlus.exe")) { throw 'Extract the release ZIP before installing.' }
$target = Join-Path $env:LOCALAPPDATA 'Programs/TrafficMonitorPlus'
if (Get-Process TrafficMonitorPlus -ErrorAction SilentlyContinue) { throw 'Exit TrafficMonitorPlus from its tray menu, then retry installation.' }
New-Item -ItemType Directory -Path $target -Force | Out-Null
foreach ($file in @('TrafficMonitorPlus.exe', 'TrafficMonitorPlus.exe.config', 'OpenHardwareMonitorApi.dll', 'LibreHardwareMonitorLib.dll', 'HidSharp.dll', 'README.md', 'THIRD_PARTY_NOTICES.md', 'uninstall.bat')) {
    Copy-Item -LiteralPath (Join-Path $source $file) -Destination $target -Force
}
Copy-Item -LiteralPath (Join-Path $source 'licenses') -Destination $target -Recurse -Force
New-Item -ItemType Directory -Path (Join-Path $target 'scripts') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'sensor-diagnostics.ps1') -Destination (Join-Path $target 'scripts') -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'uninstall.ps1') -Destination (Join-Path $target 'scripts') -Force
$shell = New-Object -ComObject WScript.Shell
$shortcut = $shell.CreateShortcut((Join-Path ([Environment]::GetFolderPath('Programs')) 'TrafficMonitorPlus.lnk'))
$shortcut.TargetPath = Join-Path $target 'TrafficMonitorPlus.exe'
$shortcut.WorkingDirectory = $target
$shortcut.Save()
$desktopShortcut = $shell.CreateShortcut((Join-Path ([Environment]::GetFolderPath('Desktop')) 'Yash Monitor.lnk'))
$desktopShortcut.TargetPath = Join-Path $target 'TrafficMonitorPlus.exe'
$desktopShortcut.Arguments = '--show-settings'
$desktopShortcut.WorkingDirectory = $target
$desktopShortcut.IconLocation = (Join-Path $target 'TrafficMonitorPlus.exe') + ',0'
$desktopShortcut.Description = 'Open Yash Monitor and its settings'
$desktopShortcut.Save()
Start-Process -FilePath (Join-Path $target 'TrafficMonitorPlus.exe') -Verb RunAs
Write-Host "Installed for this user in $target"
