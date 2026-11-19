# TrafficMonitorPlus 2.0.0

This release replaces the older public 1.x application with the native C++20 taskbar monitor developed in this repository. Previous Git history and release downloads are retained.

## Downloads

- **Setup-x64.exe (recommended):** standard installation wizard, bundled Microsoft Visual C++ runtime, Start menu shortcut, optional desktop shortcut, and Windows Installed Apps uninstaller.
- **windows-x64.zip:** extract the entire folder, then run `install.bat` or the application. If the Microsoft Visual C++ x64 runtime is missing, install the signed copy in `prerequisites` first.
- **SHA256SUMS.txt:** hashes of the exact release downloads.

Requires Windows 10 version 1809 or newer / Windows 11 on an x64 PC. Administrator access enables live per-app traffic and hardware sensors. Sensor availability depends on hardware, drivers and permissions. The app installer is unsigned; no publisher signing certificate is configured.

## Highlights

- Fixed-size Quick Panel with Overview, Processes and Storage tabs.
- Scrollbar with wheel, drag and keyboard navigation.
- Live TCP/UDP upload and download rankings; daily tracked history.
- Confirmed close actions for live network apps and exact service selection for service hosts.
- Compact daily data tooltip, CPU/RAM/GPU/battery monitoring, settings and startup controls.
- Installer and portable packages include the sensor DLLs, configuration, license notices and runtime prerequisite.

## Validation

Deterministic native tests run in CI. Interactive desktop/compositor checks are labelled separately and are not represented as passing in a headless runner. In the most recent local full run, 18/19 passed; the separate taskbar/fullscreen interaction fixture failed. The new live close/cancel/group identity tests passed using only test-owned child processes. Live UDP capture matched 600,000 sent and 600,000 received bytes in the controlled fixture.

The release workflow also installs and uninstalls its generated Setup EXE in an isolated runner folder before uploading release assets. A broad Windows/hardware compatibility matrix and publisher signing are not yet completed.
