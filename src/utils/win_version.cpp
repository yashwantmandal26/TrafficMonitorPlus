// TrafficMonitorPlus - Windows Version Detection
#include "pch.h"
#include "utils/win_version.h"

namespace tmp {

// RtlGetVersion signature (ntdll.dll, always present)
using RtlGetVersionPtr = NTSTATUS(WINAPI*)(PRTL_OSVERSIONINFOW);

WinVersionInfo GetWindowsVersion() {
    WinVersionInfo info{};

    // Use RtlGetVersion for accurate version (bypasses compatibility manifests)
    HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
    if (ntdll) {
        auto rtl_get_version = reinterpret_cast<RtlGetVersionPtr>(
            ::GetProcAddress(ntdll, "RtlGetVersion"));
        if (rtl_get_version) {
            RTL_OSVERSIONINFOW osvi{};
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            if (rtl_get_version(&osvi) == 0 /* STATUS_SUCCESS */) {
                info.major = osvi.dwMajorVersion;
                info.minor = osvi.dwMinorVersion;
                info.build = osvi.dwBuildNumber;
            }
        }
    }

    info.is_win11 = (info.build >= 22000);
    info.is_win10 = (info.build >= 10240 && info.build < 22000);
    info.is_dark_theme = IsSystemDarkTheme();

    return info;
}

bool IsSystemDarkTheme() {
    DWORD value = 1; // default to light theme
    DWORD size = sizeof(value);
    LSTATUS status = ::RegGetValueW(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme",
        RRF_RT_REG_DWORD,
        nullptr,
        &value,
        &size);

    if (status != ERROR_SUCCESS) {
        return false; // assume light theme on error
    }

    // 0 = dark theme, 1 = light theme
    return (value == 0);
}

} // namespace tmp
