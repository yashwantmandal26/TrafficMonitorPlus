# TrafficMonitorPlus

A lightweight Windows taskbar monitor with **Quick Panel**, live per-app network traffic, CPU/RAM/GPU readings and storage information.

## Download and install

[**Download Setup EXE (recommended)**](https://github.com/yashwantmandal26/TrafficMonitorPlus/releases/latest/download/TrafficMonitorPlus-2.0.0-Setup-x64.exe) · [Portable ZIP](https://github.com/yashwantmandal26/TrafficMonitorPlus/releases/latest/download/TrafficMonitorPlus-2.0.0-windows-x64.zip) · [All releases](https://github.com/yashwantmandal26/TrafficMonitorPlus/releases)

1. Download **TrafficMonitorPlus-2.0.0-Setup-x64.exe** from the latest release.
2. Run the setup wizard and approve the Windows administrator prompt. The required Microsoft Visual C++ runtime is bundled.
3. Launch TrafficMonitorPlus from the Start menu. Click the taskbar monitor to open Quick Panel; right-click for Settings and Exit.

Requires **Windows 10 version 1809 or later / Windows 11, x64**. Native ARM64 and 32-bit builds are not provided. The installer is currently unsigned; no verified publisher signing certificate is configured. Hardware sensor availability varies by device and driver.

### Portable option

Extract the **entire ZIP**. Keep the application, its `.exe.config`, and all three sensor DLLs together. If the Microsoft Visual C++ x64 runtime is not installed, run the signed installer in `prerequisites` first. Launch `TrafficMonitorPlus.exe`, or use `install.bat` for the legacy per-user installation method.

### Uninstall and upgrades

Use Windows **Installed apps → TrafficMonitorPlus → Uninstall** for the Setup version. The installer upgrades the same installation directory. Exit the running app before manually replacing portable files. User settings and history are retained in `%LOCALAPPDATA%\TrafficMonitorPlus`; the installer does not replace them with another user's configuration.

## Quick Panel

- **Overview:** All activity, Network and CPU & RAM filters.
- **Processes:** live CPU/memory sorting and process IDs.
- **Storage:** drive capacities, free space and click-to-open.
- Visible scrollbar with wheel, thumb drag, arrows, Page Up/Page Down and Home/End. Header and action buttons stay fixed.
- Uploading/downloading rows show current app speeds. Their **×** buttons open a confirmation; browser process groups and specific Windows services are verified before closing/stopping.
- Daily per-app totals use Windows records when available, with clearly labelled **tracked since** history otherwise. Traffic from before tracking started is not invented.
- Pin keeps the panel open. Esc closes it. `Win + Shift + M` opens Quick Panel.

Live per-app traffic and some hardware sensors need administrator access. Protected system processes and failed service stop requests are reported rather than bypassed. Temperatures and battery power can be unavailable on unsupported hardware. [Measurement details](docs/TELEMETRY_ACCURACY.md).

## Build from source

Install Visual Studio 2022 **Desktop development with C++**, CMake and a Windows SDK. Build x64:

```powershell
cmake -S . -B out -A x64 -DBUILD_TESTING=ON
cmake --build out --config Release --parallel 4
ctest --test-dir out -C Release --output-on-failure
```

To build both the Setup EXE and portable ZIP, install Inno Setup 6 and run `./scripts/package-release.ps1`. This verifies the bundled Microsoft runtime signature and writes release packages plus `SHA256SUMS.txt` to `dist`.

GitHub Actions builds on `windows-2022`, runs deterministic tests and smoke-tests the generated installer/uninstaller before adding assets to a tagged draft release. Desktop/compositor tests carry the `desktop` label and require an interactive Windows desktop; CI uses `ctest -LE desktop` and does not claim those checks passed.

## Validation and licensing

[Version 2.0.0 release notes and limitations](docs/RELEASE_2.0.0.md). This release has not been certified across every Windows/hardware combination. The most recent local full test run passed 18/19; the separate taskbar/fullscreen interaction fixture failed. New network close/cancel and PID identity tests passed using only test-owned child processes.

[Project license](LICENSE) · [Third-party notices](THIRD_PARTY_NOTICES.md) · [Bundled dependency licenses](licenses/).

Version 2 replaces the older public 1.x implementation. Previous Git history and earlier releases are retained.
