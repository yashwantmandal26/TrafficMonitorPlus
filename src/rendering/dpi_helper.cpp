// TrafficMonitorPlus - DPI Awareness Helper Implementation
#include "pch.h"
#include "rendering/dpi_helper.h"

#include <shellscalingapi.h>

namespace tmp {

void DpiHelper::Init() {
    if (s_initialized) return;
    s_initialized = true;

    // Try to load SetProcessDpiAwarenessContext from user32.dll (Win10 1703+)
    HMODULE user32 = ::GetModuleHandleW(L"user32.dll");
    if (user32) {
        auto setDpiCtx = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
            ::GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (setDpiCtx) {
            setDpiCtx(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }

        // Cache GetDpiForWindow for later use
        s_getDpiForWindow = reinterpret_cast<GetDpiForWindowFn>(
            ::GetProcAddress(user32, "GetDpiForWindow"));
    }

    // Fallback: try SetProcessDpiAwareness from shcore.dll (Win8.1+)
    if (!s_getDpiForWindow) {
        HMODULE shcore = ::LoadLibraryW(L"shcore.dll");
        if (shcore) {
            using SetProcessDpiAwarenessFn = HRESULT(WINAPI*)(PROCESS_DPI_AWARENESS);
            auto setDpiAwareness = reinterpret_cast<SetProcessDpiAwarenessFn>(
                ::GetProcAddress(shcore, "SetProcessDpiAwareness"));
            if (setDpiAwareness) {
                setDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
            }
        }
    }
}

UINT DpiHelper::GetDpiForWindow(HWND hwnd) {
    // Use cached function pointer if available
    if (s_getDpiForWindow && hwnd) {
        UINT dpi = s_getDpiForWindow(hwnd);
        if (dpi > 0) return dpi;
    }

    // Fallback: use device caps
    HDC hdc = ::GetDC(hwnd);
    if (!hdc) return 96;

    int dpi = ::GetDeviceCaps(hdc, LOGPIXELSX);
    ::ReleaseDC(hwnd, hdc);
    return (dpi > 0) ? static_cast<UINT>(dpi) : 96;
}

float DpiHelper::GetScaleFactor(HWND hwnd) {
    return static_cast<float>(GetDpiForWindow(hwnd)) / 96.0f;
}

int DpiHelper::Scale(int value, HWND hwnd) {
    float factor = GetScaleFactor(hwnd);
    return static_cast<int>(std::round(static_cast<float>(value) * factor));
}

float DpiHelper::ScaleF(float value, HWND hwnd) {
    return value * GetScaleFactor(hwnd);
}

} // namespace tmp
