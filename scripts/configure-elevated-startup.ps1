param([string]$ExePath = (Join-Path (Split-Path -Parent $PSScriptRoot) 'TrafficMonitorPlus.exe'))
$ErrorActionPreference = 'Stop'
$reportPath = Join-Path $env:LOCALAPPDATA 'TrafficMonitorPlus\elevated-startup-setup.json'
try {
    $resolvedExe = (Resolve-Path -LiteralPath $ExePath).Path
    $setup = Start-Process -FilePath $resolvedExe -ArgumentList '--configure-startup' -WindowStyle Hidden -PassThru -Wait
    if ($setup.ExitCode -ne 0) { throw "Startup configuration failed: $($setup.ExitCode)" }
    $sid = [Security.Principal.WindowsIdentity]::GetCurrent().User.Value
    $task = Get-ScheduledTask -TaskName ('TrafficMonitorPlus-' + $sid)
    if ($task.Principal.RunLevel -ne 'Highest' -or $task.Actions.Execute -ne $resolvedExe) {
        throw 'Startup task did not match the requested executable and administrator privileges.'
    }
    [PSCustomObject]@{
        Success = $true
        Executable = $task.Actions.Execute
        RunLevel = [string]$task.Principal.RunLevel
        LogonType = [string]$task.Principal.LogonType
        TaskName = $task.TaskName
        BatteryStartAllowed = -not $task.Settings.DisallowStartIfOnBatteries
        BatteryOperationAllowed = -not $task.Settings.StopIfGoingOnBatteries
        ExecutionTimeLimit = $task.Settings.ExecutionTimeLimit
    } | ConvertTo-Json | Set-Content -LiteralPath $reportPath -Encoding UTF8
} catch {
    [PSCustomObject]@{ Success = $false; Error = $_.Exception.Message } |
        ConvertTo-Json | Set-Content -LiteralPath $reportPath -Encoding UTF8
    exit 1
}
