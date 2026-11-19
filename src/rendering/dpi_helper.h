// TrafficMonitorPlus - DPI Awareness Helper
#pragma once

namespace tmp {

/// Static helper for per-monitor DPI awareness.
/// Dynamically loads functions to maintain compatibility with older Win10 builds.
class DpiHelper {
public:
    /// Call once at startup before any window creation.
    /// Enables per-monitor DPI awareness v2 (falls back to v1 if unavailable).
    static void Init();

    /// Returns the DPI for the given window.
    /// Falls back to GetDeviceCaps if GetDpiForWindow is unavailable.
    static UINT GetDpiForWindow(HWND hwnd);

    /// Returns scale factor relative to 96 DPI (1.0 = 100%).
    static float GetScaleFactor(HWND hwnd);

    /// Scales an integer value by the window's DPI factor.
    static int Scale(int value, HWND hwnd);

    /// Scales a float value by the window's DPI factor.
    static float ScaleF(float value, HWND hwnd);

private:
    DpiHelper() = delete;

    // Cached function pointers (resolved once in Init)
    using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
    using GetDpiForWindowFn = UINT(WINAPI*)(HWND);

    static inline GetDpiForWindowFn s_getDpiForWindow = nullptr;
    static inline bool s_initialized = false;
};

} // namespace tmp
