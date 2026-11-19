#ifndef AppVersion
  #define AppVersion "2.0.0"
#endif
#ifndef StageDir
  #error StageDir is required
#endif
#ifndef OutputDir
  #define OutputDir "..\dist"
#endif
[Setup]
AppId={{6B0B760B-0122-4CF2-9C14-53F4F6A60F3C}
AppName=TrafficMonitorPlus
AppVersion={#AppVersion}
AppPublisher=Yashwant Mandal
AppPublisherURL=https://github.com/yashwantmandal26/TrafficMonitorPlus
AppSupportURL=https://github.com/yashwantmandal26/TrafficMonitorPlus/issues
AppUpdatesURL=https://github.com/yashwantmandal26/TrafficMonitorPlus/releases/latest
DefaultDirName={autopf}\TrafficMonitorPlus
DefaultGroupName=TrafficMonitorPlus
UninstallDisplayIcon={app}\TrafficMonitorPlus.exe
OutputDir={#OutputDir}
OutputBaseFilename=TrafficMonitorPlus-{#AppVersion}-Setup-x64
SetupIconFile=..\res\icon.ico
LicenseFile=..\LICENSE
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0.17763
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
RestartApplications=no
SetupLogging=yes
[Tasks]
Name: desktopicon; Description: "Create a desktop shortcut"; Flags: unchecked
[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Excludes: "prerequisites\*"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\prerequisites\vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall; AfterInstall: InstallRuntime
[Icons]
Name: "{group}\TrafficMonitorPlus"; Filename: "{app}\TrafficMonitorPlus.exe"; Parameters: "--show-settings"; WorkingDir: "{app}"
Name: "{group}\Uninstall TrafficMonitorPlus"; Filename: "{uninstallexe}"
Name: "{commondesktop}\TrafficMonitorPlus"; Filename: "{app}\TrafficMonitorPlus.exe"; Parameters: "--show-settings"; WorkingDir: "{app}"; Tasks: desktopicon
[Run]
Filename: "{app}\TrafficMonitorPlus.exe"; Description: "Launch TrafficMonitorPlus"; Flags: postinstall skipifsilent nowait runascurrentuser
[UninstallRun]
Filename: "{app}\TrafficMonitorPlus.exe"; Parameters: "--exit-running"; Flags: runhidden waituntilterminated skipifdoesntexist
Filename: "{app}\TrafficMonitorPlus.exe"; Parameters: "--remove-startup"; Flags: runhidden waituntilterminated skipifdoesntexist
[Code]
procedure InstallRuntime();
var ResultCode: Integer;
begin
  if not Exec(ExpandConstant('{tmp}\vc_redist.x64.exe'), '/install /quiet /norestart', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
    RaiseException('Could not start the Microsoft Visual C++ runtime installer.');
  if (ResultCode <> 0) and (ResultCode <> 1638) and (ResultCode <> 3010) then
    RaiseException(Format('Microsoft Visual C++ runtime setup failed (code %d).', [ResultCode]));
end;
