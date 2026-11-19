$ErrorActionPreference = 'Stop'
if (Get-Process TrafficMonitorPlus -ErrorAction SilentlyContinue) { throw 'Exit TrafficMonitorPlus from its tray menu before uninstalling.' }
$sid = [Security.Principal.WindowsIdentity]::GetCurrent().User.Value
$taskName = 'TrafficMonitorPlus-' + $sid
$task = Get-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
if ($task) { Unregister-ScheduledTask -TaskName $taskName -Confirm:$false }
Remove-ItemProperty -LiteralPath 'HKCU:/Software/Microsoft/Windows/CurrentVersion/Run' -Name TrafficMonitorPlus -ErrorAction SilentlyContinue
$shortcut = Join-Path ([Environment]::GetFolderPath('Programs')) 'TrafficMonitorPlus.lnk'
if (Test-Path -LiteralPath $shortcut) { Remove-Item -LiteralPath $shortcut }
$desktopShortcut = Join-Path ([Environment]::GetFolderPath('Desktop')) 'Yash Monitor.lnk'
if (Test-Path -LiteralPath $desktopShortcut) { Remove-Item -LiteralPath $desktopShortcut }
Write-Host 'Startup task and shortcut removed. Application files, settings and usage history are retained.'
