// TrafficMonitorPlus - Application Settings
#pragma once
#include "../pch.h"
#include "../types.h"

namespace tmp {

// All application settings with sane defaults.
struct Settings {
    // ─── Display ───────────────────────────────────────────
    DisplayMode display_mode = DisplayMode::Taskbar;
    std::map<DisplayItem, DisplayItemConfig> items; // initialized via GetDefaults()
    LayoutDirection layout = LayoutDirection::AutoCompact;
    bool compact = true;

    // ─── Appearance ────────────────────────────────────────
    std::wstring theme = L"auto";  // "auto", "dark", "light"
    std::wstring font_family = L"Segoe UI";
    int font_size = 13;
    bool font_bold = false;
    bool thermal_colors = true;
    bool show_unit_signs = true;
    int stat_value_gap = 0;
    int item_spacing = 5;
    int vertical_margin = 2;
    int taskbar_offset_x = 11;
    int taskbar_offset_y = 2;
    std::map<DisplayItem, COLORREF> custom_label_colors;  // empty = use theme defaults
    std::map<DisplayItem, COLORREF> custom_value_colors;
    COLORREF background_color = RGB(0, 0, 0); // 0,0,0 = transparent/auto

    // ─── Behavior ──────────────────────────────────────────
    bool auto_start = true;
    int poll_interval_ms = 1000;
    bool hide_on_fullscreen = true;
    bool speed_unit_bits = false; // false=bytes, true=bits
    std::wstring speed_unit = L"auto"; // "auto", "KB/s", "MB/s"

    // ─── Floating widget ───────────────────────────────────
    int widget_x = -1;  // -1 = auto position
    int widget_y = -1;
    bool widget_click_through = false;
    int widget_opacity = 90; // 0-100

    // ─── Notifications ────────────────────────────────────
    bool notifications_enabled = false;
    int cpu_alert_threshold = 90;
    int cpu_alert_sustained_sec = 30;
    int ram_alert_threshold = 95;
    int temp_alert_threshold = 90;
    int gpu_color_threshold = 90;
    int download_color_threshold_mb = 10;
    int upload_color_threshold_mb = 4;
    int battery_color_threshold_w = 65;
    int frequency_color_threshold_ghz = 5;

    // ─── Network ──────────────────────────────────────────
    bool auto_select_adapter = true;
    std::wstring adapter_name;

    // ─── Advanced ─────────────────────────────────────────
    bool check_updates = true;

    // ─── Methods ──────────────────────────────────────────

    // Load settings from a JSON file. Returns defaults if file doesn't exist or is invalid.
    static Settings LoadFromFile(const fs::path& path);

    // Save settings to a JSON file. Returns true on success.
    bool SaveToFile(const fs::path& path) const;

    // Returns the saved Yash Monitor preset, including all metric selections.
    static Settings GetDefaults();
    static Settings MergeDraft(const Settings& live, const Settings& original, const Settings& draft);

    // Returns enabled items sorted by their order field.
    std::vector<std::pair<DisplayItem, DisplayItemConfig>> GetEnabledItemsSorted() const;

    bool operator==(const Settings& other) const;
    bool operator!=(const Settings& other) const { return !(*this == other); }
};

} // namespace tmp
