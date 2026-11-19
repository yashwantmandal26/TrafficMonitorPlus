param([string]$RuntimeDirectory = (Split-Path -Parent $PSScriptRoot))
$ErrorActionPreference = 'Stop'
$runtime = (Resolve-Path -LiteralPath $RuntimeDirectory).Path
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
Write-Output "Elevated: $($principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator))"
[Reflection.Assembly]::LoadFrom((Join-Path $runtime 'HidSharp.dll')) | Out-Null
[Reflection.Assembly]::LoadFrom((Join-Path $runtime 'LibreHardwareMonitorLib.dll')) | Out-Null
$computer = New-Object LibreHardwareMonitor.Hardware.Computer
$computer.IsCpuEnabled = $true
$computer.IsGpuEnabled = $true
function Read-Hardware($hardware) {
    $hardware.Update()
    foreach ($sensor in $hardware.Sensors) {
        if ($null -ne $sensor.Value) {
            [pscustomobject]@{ Hardware = $hardware.Name; Type = $sensor.SensorType; Sensor = $sensor.Name; Value = $sensor.Value }
        }
    }
    foreach ($child in $hardware.SubHardware) { Read-Hardware $child }
}
try {
    $computer.Open()
    foreach ($hardware in $computer.Hardware) { Read-Hardware $hardware | Out-Null }
    Start-Sleep -Seconds 1
    foreach ($hardware in $computer.Hardware) { Read-Hardware $hardware }
} finally { $computer.Close() }
