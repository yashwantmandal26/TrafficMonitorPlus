// TrafficMonitorPlus - Windows Version Detection
#pragma once
#include "../pch.h"
#include "../types.h"

namespace tmp {

// Detects the real Windows version using RtlGetVersion (not subject to
// compatibility shims) and reads the current system dark/light theme setting.
WinVersionInfo GetWindowsVersion();

// Returns true if the system-wide dark theme is active.
// Reads HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Themes\Personalize\AppsUseLightTheme
bool IsSystemDarkTheme();

} // namespace tmp
