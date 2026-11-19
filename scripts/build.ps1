param([switch]$SkipTests)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio with the Desktop development with C++ workload.' }
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual Studio C++ tools were not found.' }
$bin = Join-Path $vs 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin'
function Invoke-Checked($exe, $arguments) {
    & $exe @arguments
    if ($LASTEXITCODE -ne 0) { throw "$exe failed with exit code $LASTEXITCODE" }
}
Invoke-Checked "$bin/cmake.exe" @('-S', $root, '-B', "$root/out", '-G', 'Visual Studio 17 2022', '-A', 'x64', '-DBUILD_TESTING=ON')
Invoke-Checked "$bin/cmake.exe" @('--build', "$root/out", '--config', 'Release', '--parallel')
if (!$SkipTests) { Invoke-Checked "$bin/ctest.exe" @('--test-dir', "$root/out", '-C', 'Release', '--output-on-failure') }
Invoke-Checked "$bin/cpack.exe" @('--config', "$root/out/CPackConfig.cmake", '-C', 'Release', '-B', "$root/dist")
Get-ChildItem -LiteralPath "$root/dist" -Filter '*.zip' | Get-FileHash -Algorithm SHA256 |
    ForEach-Object { '{0}  {1}' -f $_.Hash, (Split-Path -Leaf $_.Path) } |
    Set-Content -LiteralPath "$root/dist/SHA256SUMS.txt"
Write-Host "Release package: $root/dist"
