param([string]$BuildDir='out', [switch]$SkipBuild, [switch]$SkipInstaller)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root $BuildDir
$dist=Join-Path $root 'dist'
$stage=Join-Path $build 'release-stage'
if(!((Get-Content (Join-Path $root 'CMakeLists.txt') -Raw) -match 'project\(TrafficMonitorPlus VERSION ([0-9.]+)')){throw 'Project version missing.'}
$version=$Matches[1]
function Checked($command,$arguments){& $command @arguments;if($LASTEXITCODE -ne 0){throw "$command failed: $LASTEXITCODE"}}
if(!$SkipBuild){
 Checked cmake @('-S',$root,'-B',$build,'-A','x64','-DBUILD_TESTING=ON')
 Checked cmake @('--build',$build,'--config','Release','--parallel','4')
 Checked ctest @('--test-dir',$build,'-C','Release','-LE','desktop','--output-on-failure')
}
New-Item -ItemType Directory -Path $dist,$stage -Force | Out-Null
Checked cmake @('--install',$build,'--config','Release','--prefix',$stage)
$prereq=Join-Path $stage 'prerequisites'
New-Item -ItemType Directory -Path $prereq -Force | Out-Null
$runtime=Join-Path $prereq 'vc_redist.x64.exe'
Invoke-WebRequest 'https://aka.ms/vc14/vc_redist.x64.exe' -OutFile $runtime
$signature=Get-AuthenticodeSignature -LiteralPath $runtime
if($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'Microsoft Corporation'){throw 'Microsoft runtime signature verification failed.'}
foreach($name in @('TrafficMonitorPlus.exe','TrafficMonitorPlus.exe.config','OpenHardwareMonitorApi.dll','LibreHardwareMonitorLib.dll','HidSharp.dll','LICENSE','install.bat','scripts\install.ps1','prerequisites\vc_redist.x64.exe')){
 if(!(Test-Path -LiteralPath (Join-Path $stage $name))){throw "Release is missing $name"}
}
$zip=Join-Path $dist "TrafficMonitorPlus-$version-windows-x64.zip"
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -Force
if(!$SkipInstaller){
 $compiler=Get-Command ISCC.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -First 1
 if(!$compiler){$compiler="${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"}
 if(!(Test-Path -LiteralPath $compiler)){throw 'Inno Setup 6 is required; use the GitHub Actions release workflow.'}
 Checked $compiler @("/DAppVersion=$version","/DStageDir=$stage","/DOutputDir=$dist",(Join-Path $root 'installer\TrafficMonitorPlus.iss'))
}
Get-ChildItem -LiteralPath $dist -File | Where-Object Name -Match "^TrafficMonitorPlus-$([regex]::Escape($version)).*\.(zip|exe)$" | Sort-Object Name | Get-FileHash -Algorithm SHA256 | ForEach-Object {"$($_.Hash.ToLowerInvariant())  $(Split-Path -Leaf $_.Path)"} | Set-Content (Join-Path $dist 'SHA256SUMS.txt')
Write-Output "Release packages ready in $dist"
