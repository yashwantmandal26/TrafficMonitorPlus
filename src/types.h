// TrafficMonitorPlus - Shared Types
// All subsystems reference these common types.
#pragma once
#include "pch.h"
#include <limits>

namespace tmp {

// ─── Version ───────────────────────────────────────────
constexpr int VERSION_MAJOR = 2;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;
constexpr const wchar_t* APP_NAME       = L"TrafficMonitorPlus";
constexpr const wchar_t* APP_MUTEX_NAME = L"Global\\TrafficMonitorPlus_SingleInstance";
constexpr const wchar_t* CONFIG_FILE    = L"config.json";

// ─── Display Items ─────────────────────────────────────
enum class DisplayItem : uint32_t {
    DownloadSpeed   = 0,
    UploadSpeed     = 1,
    CpuUsage        = 2,
    CpuTemp         = 3,
    RamUsage        = 4,
    GpuUsage        = 5,
    GpuTemp         = 6,
    BatteryWattage  = 7,
    CpuFrequency    = 8,
    _Count
};

constexpr size_t DISPLAY_ITEM_COUNT = static_cast<size_t>(DisplayItem::_Count);

// Human-readable default labels for each display item
inline const wchar_t* GetDefaultLabel(DisplayItem item) {
    switch (item) {
        case DisplayItem::DownloadSpeed:  return L"\u2193";     // ↓ 
        case DisplayItem::UploadSpeed:    return L"\u2191";     // ↑ 
        case DisplayItem::CpuUsage:       return L"CPU:";
        case DisplayItem::CpuTemp:        return L"";           // shown as °C suffix (e.g. 45°C)
        case DisplayItem::RamUsage:       return L"RAM:";
        case DisplayItem::GpuUsage:       return L"dGPU:";
        case DisplayItem::GpuTemp:        return L"";
        case DisplayItem::BatteryWattage: return L"\u26A1";     // ⚡
        case DisplayItem::CpuFrequency:   return L"Freq:";
        default: return L"";
    }
}

// Configuration for a single display item
struct DisplayItemConfig {
    bool enabled = false;
    int order = 0;                // Sort order for display
    std::wstring custom_label;    // Empty = use default label
    int font_size = 0;            // 0 = global font
    int value_width = 0;          // Stable value slot in DIPs; 0 = measured preset
    int label_gap = 0;            // Custom gap between stat label and value (0 = use global)
    int offset_x = 0;             // Custom horizontal shift/move for this individual element
    bool show_unit = true;        // Show unit sign (%, °C, W, GHz, etc.) for this individual metric
};

inline const char* GetItemKey(DisplayItem item) {
    switch (item) {
        case DisplayItem::DownloadSpeed:  return "download_speed";
        case DisplayItem::UploadSpeed:    return "upload_speed";
        case DisplayItem::CpuUsage:       return "cpu_usage";
        case DisplayItem::CpuTemp:        return "cpu_temp";
        case DisplayItem::RamUsage:       return "ram_usage";
        case DisplayItem::GpuUsage:       return "gpu_usage";
        case DisplayItem::GpuTemp:        return "gpu_temp";
        case DisplayItem::BatteryWattage: return "battery_wattage";
        case DisplayItem::CpuFrequency:   return "cpu_frequency";
        default: return "unknown";
    }
}

// ─── Telemetry Snapshot ────────────────────────────────
// Populated by TelemetryEngine every poll cycle, consumed by renderers.
struct TelemetryData {
    // Network
    double download_bytes_per_sec = -1.0;
    double upload_bytes_per_sec   = -1.0;
    uint64_t total_session_download = 0;
    uint64_t total_session_upload   = 0;

    // CPU
    double cpu_usage_percent = -1.0;   // 0-100
    double cpu_temp_celsius  = -1.0;   // °C, -1 if unavailable
    double cpu_freq_ghz      = -1.0;  // GHz, -1 if unavailable

    // Memory
    double ram_usage_percent = -1.0;   // 0-100
    uint64_t ram_used_bytes  = 0;
    uint64_t ram_total_bytes = 0;

    // GPU
    double gpu_usage_percent = -1.0;
    double gpu_temp_celsius  = -1.0;
    std::wstring gpu_prefix  = L"dGPU:";

    // Battery
    double battery_wattage   = std::numeric_limits<double>::quiet_NaN(); // Positive charging, negative discharging; NaN unavailable.
    int    battery_percent   = -1;    // -1 if no battery

    // Timestamp
    std::chrono::steady_clock::time_point timestamp;
};

// ─── Process Info (for Top Hogs) ───────────────────────
struct ProcessInfo {
    DWORD    pid = 0;
    std::wstring name;
    double   cpu_percent = 0.0;
    double   bandwidth_bytes_per_sec = 0.0;
    uint64_t memory_bytes = 0;
};

// ─── Thermal Color Breakpoints ─────────────────────────
enum class ThermalLevel {
    Cool,       // Normal operating temperature / low usage
    Warm,       // Starting to heat up / moderate usage
    Hot,        // High temperature / high usage
    Critical    // Danger zone / near-maximum usage
};

// Returns 0.0 (cool) to 1.0 (critical) heat factor for temperature
inline float GetTempHeatFactor(double celsius) {
    if (celsius <= 30.0)  return 0.0f;
    if (celsius >= 85.0)  return 1.0f;
    return static_cast<float>((celsius - 30.0) / 55.0);
}

// Returns 0.0 (cool) to 1.0 (critical) heat factor for usage percentage
inline float GetUsageHeatFactor(double percent) {
    if (percent <= 50.0)  return 0.0f;
    if (percent >= 95.0)  return 1.0f;
    return static_cast<float>((percent - 50.0) / 45.0);
}

// ─── Network Quality Level ─────────────────────────────
enum class NetQuality {
    Good,       // 🟢 avg <50ms, loss <2%, jitter <15ms
    Fair,       // 🟡 avg <150ms, loss <5%, jitter <30ms
    Poor,       // 🔴 everything else
    Unknown     // Not yet measured
};

// ─── Display Mode ──────────────────────────────────────
enum class DisplayMode {
    None = -1,
    Taskbar,    // Embedded in Windows taskbar
    Floating,   // Floating desktop capsule widget
    Both        // Both simultaneously
};

// ─── Layout Direction ──────────────────────────────────
enum class LayoutDirection {
    Horizontal,
    Vertical,
    TwoRows,
    Stacked,
    AutoCompact
};

// ─── Windows Version Info ──────────────────────────────
struct WinVersionInfo {
    DWORD major = 0;
    DWORD minor = 0;
    DWORD build = 0;
    bool  is_win11   = false;    // Build >= 22000
    bool  is_win10   = false;    // Build >= 10240 && < 22000
    bool  is_dark_theme = false;
};

// ─── Custom Messages ───────────────────────────────────
constexpr UINT WM_TMP_TRAYICON      = WM_APP + 1;
constexpr UINT WM_TMP_TELEMETRY     = WM_APP + 2;  // wParam = 0, lParam = TelemetryData*
constexpr UINT WM_TMP_THEME_CHANGED = WM_APP + 3;
constexpr UINT WM_TMP_SETTINGS      = WM_APP + 4;
constexpr UINT WM_TMP_QUIT          = WM_APP + 5;
constexpr const wchar_t* SHOW_SETTINGS_MESSAGE = L"TrafficMonitorPlus.ShowSettings";

// ─── Tray Menu IDs ─────────────────────────────────────
constexpr UINT IDM_SHOW_TASKBAR     = 40001;
constexpr UINT IDM_SHOW_FLOATING    = 40002;
constexpr UINT IDM_SETTINGS         = 40003;
constexpr UINT IDM_COPY_IP          = 40004;
constexpr UINT IDM_TASK_MANAGER     = 40006;
constexpr UINT IDM_AUTOSTART        = 40008;
constexpr UINT IDM_EXIT             = 40009;
constexpr UINT IDM_SENSOR_STATUS    = 40011;
constexpr UINT IDM_SENSOR_ADMIN     = 40012;
constexpr UINT IDM_DATA_USAGE       = 40013;

// Display item toggle menu IDs (base + DisplayItem enum value)
constexpr UINT IDM_DISPLAY_ITEM_BASE = 41000;

} // namespace tmp
