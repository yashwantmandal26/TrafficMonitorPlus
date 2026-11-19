// TrafficMonitorPlus - Application Settings (JSON serialization)
#include "pch.h"
#include "config/settings.h"
#include "utils/string_utils.h"
#include "../../lib/json.hpp"

using json = nlohmann::json;

namespace tmp {

// ─── Helper: DisplayItem ↔ string ─────────────────────────────
static DisplayItem DisplayItemFromKey(const std::string& key) {
    for (uint32_t i = 0; i < DISPLAY_ITEM_COUNT; ++i) {
        auto item = static_cast<DisplayItem>(i);
        if (GetItemKey(item) == key) return item;
    }
    return DisplayItem::_Count; // invalid key
}

// ─── Helper: DisplayMode ↔ string ─────────────────────────────
static std::string DisplayModeToStr(DisplayMode m) {
    switch (m) {
        case DisplayMode::None: return "none";
        case DisplayMode::Taskbar:  return "taskbar";
        case DisplayMode::Floating: return "floating";
        case DisplayMode::Both:     return "both";
        default: return "taskbar";
    }
}

static DisplayMode DisplayModeFromStr(const std::string& s) {
    if (s == "none") return DisplayMode::None;
    if (s == "floating") return DisplayMode::Floating;
    if (s == "both")     return DisplayMode::Both;
    return DisplayMode::Taskbar;
}

// ─── Helper: LayoutDirection ↔ string ──────────────────────────
static std::string LayoutDirToStr(LayoutDirection d) {
    if(d==LayoutDirection::AutoCompact)return "auto_compact";
    if(d==LayoutDirection::Stacked)return "stacked";
    switch (d) {
        case LayoutDirection::Vertical: return "vertical";
        case LayoutDirection::TwoRows:  return "two_rows";
        default:                        return "horizontal";
    }
}

static LayoutDirection LayoutDirFromStr(const std::string& s) {
    if(s=="auto_compact")return LayoutDirection::AutoCompact;
    if(s=="stacked")return LayoutDirection::Stacked;
    if (s == "vertical") return LayoutDirection::Vertical;
    if (s == "two_rows" || s == "two_lines" || s == "double_line") return LayoutDirection::TwoRows;
    return LayoutDirection::Horizontal;
}

// ─── Helper: COLORREF ↔ JSON (as "#RRGGBB" string) ────────────
static std::string ColorToHex(COLORREF c) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X",
        GetRValue(c), GetGValue(c), GetBValue(c));
    return buf;
}

static COLORREF HexToColor(const std::string& hex) {
    if (hex.size() < 7 || hex[0] != '#') return RGB(0, 0, 0);
    unsigned int r = 0, g = 0, b = 0;
    std::sscanf(hex.c_str() + 1, "%02x%02x%02x", &r, &g, &b);
    return RGB(r, g, b);
}

// ─── Helper: color map serialization ───────────────────────────
static json ColorMapToJson(const std::map<DisplayItem, COLORREF>& m) {
    json j = json::object();
    for (const auto& [item, color] : m) {
        j[GetItemKey(item)] = ColorToHex(color);
    }
    return j;
}

static std::map<DisplayItem, COLORREF> ColorMapFromJson(const json& j) {
    std::map<DisplayItem, COLORREF> m;
    if (!j.is_object()) return m;
    for (auto& [key, val] : j.items()) {
        if (val.is_string()) {
            DisplayItem item = DisplayItemFromKey(key);
            if (item != DisplayItem::_Count) m[item] = HexToColor(val.get<std::string>());
        }
    }
    return m;
}

// ─── to_json / from_json for DisplayItemConfig ─────────────────
static json ItemConfigToJson(const DisplayItemConfig& c) {
    json j;
    j["enabled"] = c.enabled;
    j["font_size"] = c.font_size;
    j["label_gap"] = c.label_gap;
    j["offset_x"] = c.offset_x;
    j["order"] = c.order;
    j["show_unit"] = c.show_unit;
    j["value_width"] = c.value_width;
    if (!c.custom_label.empty()) {
        j["custom_label"] = WideToUtf8(c.custom_label);
    }
    return j;
}

static DisplayItemConfig ItemConfigFromJson(const json& j) {
    DisplayItemConfig c;
    if (j.contains("font_size") && j["font_size"].is_number_integer()) {
        c.font_size = j["font_size"].get<int>();
        c.font_size = c.font_size == 0 ? 0 : std::clamp(c.font_size, 6, 40);
    }
    if (j.contains("value_width") && j["value_width"].is_number_integer())
        c.value_width = std::clamp(j["value_width"].get<int>(), 0, 500);
    if (j.contains("label_gap") && j["label_gap"].is_number_integer())
        c.label_gap = std::clamp(j["label_gap"].get<int>(), 0, 50);
    if (j.contains("offset_x") && j["offset_x"].is_number_integer())
        c.offset_x = std::clamp(j["offset_x"].get<int>(), -100, 200);
    if (j.contains("show_unit") && j["show_unit"].is_boolean())
        c.show_unit = j["show_unit"].get<bool>();
    if (j.contains("enabled") && j["enabled"].is_boolean())
        c.enabled = j["enabled"].get<bool>();
    if (j.contains("order") && j["order"].is_number_integer())
        c.order = j["order"].get<int>();
    if (j.contains("custom_label") && j["custom_label"].is_string())
        c.custom_label = Utf8ToWide(j["custom_label"].get<std::string>());
    return c;
}

// ─── Settings → JSON ──────────────────────────────────────────
static json SettingsToJson(const Settings& s) {
    json j;

    // Display
    j["display_mode"] = DisplayModeToStr(s.display_mode);
    {
        json items_j = json::object();
        for (const auto& [item, cfg] : s.items) {
            items_j[GetItemKey(item)] = ItemConfigToJson(cfg);
        }
        j["items"] = items_j;
    }
    j["layout"] = LayoutDirToStr(s.layout);
    j["compact"] = s.compact;

    // Appearance
    j["theme"] = WideToUtf8(s.theme);
    j["font_family"] = WideToUtf8(s.font_family);
    j["font_size"] = s.font_size;
    j["font_bold"] = s.font_bold;
    j["thermal_colors"] = s.thermal_colors;
    j["show_unit_signs"] = s.show_unit_signs;
    j["stat_value_gap"] = s.stat_value_gap;
    j["item_spacing"] = s.item_spacing;
    j["vertical_margin"] = s.vertical_margin;
    j["taskbar_offset_x"] = s.taskbar_offset_x;
    j["taskbar_offset_y"] = s.taskbar_offset_y;
    j["custom_label_colors"] = ColorMapToJson(s.custom_label_colors);
    j["custom_value_colors"] = ColorMapToJson(s.custom_value_colors);
    j["background_color"] = ColorToHex(s.background_color);

    // Behavior
    j["auto_start"] = s.auto_start;
    j["poll_interval_ms"] = s.poll_interval_ms;
    j["hide_on_fullscreen"] = s.hide_on_fullscreen;
    j["speed_unit_bits"] = s.speed_unit_bits;
    j["speed_unit"] = WideToUtf8(s.speed_unit);

    // Floating widget
    j["widget_x"] = s.widget_x;
    j["widget_y"] = s.widget_y;
    j["widget_click_through"] = s.widget_click_through;
    j["widget_opacity"] = s.widget_opacity;

    // Notifications
    j["notifications_enabled"] = s.notifications_enabled;
    j["cpu_alert_threshold"] = s.cpu_alert_threshold;
    j["cpu_alert_sustained_sec"] = s.cpu_alert_sustained_sec;
    j["ram_alert_threshold"] = s.ram_alert_threshold;
    j["temp_alert_threshold"] = s.temp_alert_threshold;
    j["gpu_color_threshold"] = s.gpu_color_threshold;
    j["download_color_threshold_mb"] = s.download_color_threshold_mb;
    j["upload_color_threshold_mb"] = s.upload_color_threshold_mb;
    j["battery_color_threshold_w"] = s.battery_color_threshold_w;
    j["frequency_color_threshold_ghz"] = s.frequency_color_threshold_ghz;

    // Network
    j["auto_select_adapter"] = s.auto_select_adapter;
    j["adapter_name"] = WideToUtf8(s.adapter_name);

    // Advanced
    j["check_updates"] = s.check_updates;

    return j;
}

// ─── JSON → Settings ──────────────────────────────────────────
// Each field is individually parsed with fallback to defaults.
static Settings SettingsFromJson(const json& j) {
    Settings s = Settings::GetDefaults();

    // Helper macros for safe extraction
    #define READ_BOOL(field) \
        if (j.contains(#field) && j[#field].is_boolean()) s.field = j[#field].get<bool>()
    #define READ_INT(field) \
        if (j.contains(#field) && j[#field].is_number_integer()) s.field = j[#field].get<int>()
    #define READ_WSTR(field) \
        if (j.contains(#field) && j[#field].is_string()) s.field = Utf8ToWide(j[#field].get<std::string>())

    // Display
    if (j.contains("display_mode") && j["display_mode"].is_string())
        s.display_mode = DisplayModeFromStr(j["display_mode"].get<std::string>());

    if (j.contains("items") && j["items"].is_object()) {
        for (auto& [key, val] : j["items"].items()) {
            DisplayItem item = DisplayItemFromKey(key);
            if (item != DisplayItem::_Count) s.items[item] = ItemConfigFromJson(val);
        }
    }

    if (j.contains("layout") && j["layout"].is_string())
        s.layout = LayoutDirFromStr(j["layout"].get<std::string>());

    READ_BOOL(compact);

    // Appearance
    READ_WSTR(theme);
    READ_WSTR(font_family);
    READ_INT(font_size);
    READ_BOOL(font_bold);
    READ_BOOL(thermal_colors);
    READ_BOOL(show_unit_signs);
    if (!s.show_unit_signs && j.contains("items") && j["items"].is_object()) {
        for (auto& [key, val] : j["items"].items()) {
            DisplayItem item = DisplayItemFromKey(key);
            if (item != DisplayItem::_Count && (!val.is_object() || !val.contains("show_unit"))) {
                s.items[item].show_unit = false;
            }
        }
    }
    READ_INT(stat_value_gap);
    READ_INT(item_spacing);
    READ_INT(vertical_margin);
    READ_INT(taskbar_offset_x);
    READ_INT(taskbar_offset_y);

    if (j.contains("custom_label_colors"))
        s.custom_label_colors = ColorMapFromJson(j["custom_label_colors"]);
    if (j.contains("custom_value_colors"))
        s.custom_value_colors = ColorMapFromJson(j["custom_value_colors"]);
    if (j.contains("background_color") && j["background_color"].is_string())
        s.background_color = HexToColor(j["background_color"].get<std::string>());

    // Behavior
    READ_BOOL(auto_start);
    READ_INT(poll_interval_ms);
    READ_BOOL(hide_on_fullscreen);
    READ_BOOL(speed_unit_bits);
    READ_WSTR(speed_unit);

    // Floating widget
    READ_INT(widget_x);
    READ_INT(widget_y);
    READ_BOOL(widget_click_through);
    READ_INT(widget_opacity);

    // Notifications
    READ_BOOL(notifications_enabled);
    READ_INT(cpu_alert_threshold);
    READ_INT(cpu_alert_sustained_sec);
    READ_INT(ram_alert_threshold);
    READ_INT(temp_alert_threshold);
    READ_INT(gpu_color_threshold);
    READ_INT(download_color_threshold_mb);
    READ_INT(upload_color_threshold_mb);
    READ_INT(battery_color_threshold_w);
    READ_INT(frequency_color_threshold_ghz);

    // Network
    READ_BOOL(auto_select_adapter);
    READ_WSTR(adapter_name);

    // Advanced
    READ_BOOL(check_updates);

    #undef READ_BOOL
    #undef READ_INT
    #undef READ_WSTR

    s.font_size = std::clamp(s.font_size, 6, 40);
    // Old quick-menu toggles could persist a hidden widget across sign-ins.
    if (s.display_mode == DisplayMode::None) s.display_mode = DisplayMode::Taskbar;
    s.poll_interval_ms = std::clamp(s.poll_interval_ms, 500, 60000);
    s.stat_value_gap = std::clamp(s.stat_value_gap, 0, 50);
    s.item_spacing = std::clamp(s.item_spacing, -100, 60);
    s.vertical_margin = std::clamp(s.vertical_margin, 0, 60);
    s.taskbar_offset_x = std::clamp(s.taskbar_offset_x, -500, 500);
    s.taskbar_offset_y = std::clamp(s.taskbar_offset_y, -100, 100);
    s.widget_opacity = std::clamp(s.widget_opacity, 10, 100);
    s.cpu_alert_threshold = std::clamp(s.cpu_alert_threshold, 1, 100);
    s.ram_alert_threshold = std::clamp(s.ram_alert_threshold, 1, 100);
    s.temp_alert_threshold = std::clamp(s.temp_alert_threshold, 1, 150);
    s.gpu_color_threshold = std::clamp(s.gpu_color_threshold, 1, 100);
    s.download_color_threshold_mb = std::clamp(s.download_color_threshold_mb, 1, 10000);
    s.upload_color_threshold_mb = std::clamp(s.upload_color_threshold_mb, 1, 10000);
    s.battery_color_threshold_w = std::clamp(s.battery_color_threshold_w, 1, 500);
    s.frequency_color_threshold_ghz = std::clamp(s.frequency_color_threshold_ghz, 1, 10);
    s.cpu_alert_sustained_sec = std::clamp(s.cpu_alert_sustained_sec, 1, 3600);
    return s;
}

// ─── Public API ────────────────────────────────────────────────

Settings Settings::MergeDraft(const Settings& live, const Settings& original, const Settings& draft) {
    auto merged = SettingsToJson(live);
    const auto before = SettingsToJson(original), after = SettingsToJson(draft);
    for (const auto& [key, value] : after.items()) {
        if (key == "items") {
            for (const auto& [item, config] : value.items())
                for (const auto& [field, changed] : config.items())
                    if (!before["items"][item].contains(field) || before["items"][item][field] != changed)
                        merged["items"][item][field] = changed;
            // An emptied custom label is omitted during serialization.
            for (const auto& [item, config] : value.items())
                if (!config.contains("custom_label") && before["items"][item].contains("custom_label"))
                    merged["items"][item].erase("custom_label");
        } else if (before[key] != value) merged[key] = value;
    }
    return SettingsFromJson(merged);
}

Settings Settings::GetDefaults() {
    Settings s;
    s.layout = LayoutDirection::AutoCompact;

    // Initialize all display items with defaults — all disabled
    for (uint32_t i = 0; i < DISPLAY_ITEM_COUNT; ++i) {
        auto item = static_cast<DisplayItem>(i);
        s.items[item] = DisplayItemConfig{ false, static_cast<int>(i), L"" };
    }

    // Default 3 columns x 2 rows (Upload/Download, CPU/GPU, CPUTemp/GPUTemp)
    s.items[DisplayItem::UploadSpeed]   = { true,  0, L"" };
    s.items[DisplayItem::DownloadSpeed] = { true,  1, L"" };
    s.items[DisplayItem::CpuUsage]      = { true,  2, L"" };
    s.items[DisplayItem::GpuUsage]      = { true,  3, L"" };
    s.items[DisplayItem::CpuTemp]       = { true,  4, L"" };
    s.items[DisplayItem::GpuTemp]       = { true,  5, L"" };

    return s;
}

Settings Settings::LoadFromFile(const fs::path& path) {
    if (!fs::exists(path)) {
        return GetDefaults();
    }

    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return GetDefaults();
        }
        json j = json::parse(file, nullptr, false);
        if (j.is_discarded()) {
            return GetDefaults();
        }
        return SettingsFromJson(j);
    }
    catch (...) {
        return GetDefaults();
    }
}

bool Settings::SaveToFile(const fs::path& path) const {
    auto failed = [&]() {
        static bool notified = false;
        if (!notified) {
            notified = true;
            std::wstring message = L"Could not save settings to:\n" + path.wstring() +
                L"\n\nCheck folder permissions and available disk space. Changes may be lost when the application closes.";
            MessageBoxW(nullptr, message.c_str(), L"Settings could not be saved", MB_OK | MB_ICONERROR);
        }
        return false;
    };
    try {
        // Ensure parent directory exists
        if (path.has_parent_path()) {
            fs::create_directories(path.parent_path());
        }

        json j = SettingsToJson(*this);

        fs::path temporary = path;
        temporary += L".tmp";
        std::ofstream file(temporary);
        if (!file.is_open()) {
            return failed();
        }

        file << j.dump(4); // pretty-print with 4-space indent
        file.flush();
        bool written = file.good();
        file.close();
        if (!written || !MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return failed();
        return true;
    }
    catch (...) {
        return failed();
    }
}

std::vector<std::pair<DisplayItem, DisplayItemConfig>> Settings::GetEnabledItemsSorted() const {
    std::vector<std::pair<DisplayItem, DisplayItemConfig>> result;
    result.reserve(DISPLAY_ITEM_COUNT);

    for (const auto& [item, cfg] : items) {
        if (cfg.enabled) {
            result.emplace_back(item, cfg);
        }
    }

    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) {
            return a.second.order < b.second.order;
        });

    return result;
}

bool Settings::operator==(const Settings& other) const {
    if (display_mode != other.display_mode) return false;
    if (layout != other.layout) return false;
    if (compact != other.compact) return false;
    if (theme != other.theme) return false;
    if (font_family != other.font_family) return false;
    if (font_size != other.font_size) return false;
    if (font_bold != other.font_bold) return false;
    if (thermal_colors != other.thermal_colors) return false;
    if (show_unit_signs != other.show_unit_signs) return false;
    if (stat_value_gap != other.stat_value_gap) return false;
    if (item_spacing != other.item_spacing) return false;
    if (vertical_margin != other.vertical_margin) return false;
    if (taskbar_offset_x != other.taskbar_offset_x) return false;
    if (taskbar_offset_y != other.taskbar_offset_y) return false;
    if (background_color != other.background_color) return false;
    if (auto_start != other.auto_start) return false;
    if (poll_interval_ms != other.poll_interval_ms) return false;
    if (hide_on_fullscreen != other.hide_on_fullscreen) return false;
    if (speed_unit_bits != other.speed_unit_bits) return false;
    if (speed_unit != other.speed_unit) return false;
    if (widget_x != other.widget_x || widget_y != other.widget_y) return false;
    if (widget_click_through != other.widget_click_through) return false;
    if (widget_opacity != other.widget_opacity) return false;
    if (notifications_enabled != other.notifications_enabled) return false;
    if (cpu_alert_threshold != other.cpu_alert_threshold) return false;
    if (cpu_alert_sustained_sec != other.cpu_alert_sustained_sec) return false;
    if (ram_alert_threshold != other.ram_alert_threshold) return false;
    if (temp_alert_threshold != other.temp_alert_threshold) return false;
    if (gpu_color_threshold != other.gpu_color_threshold) return false;
    if (download_color_threshold_mb != other.download_color_threshold_mb) return false;
    if (upload_color_threshold_mb != other.upload_color_threshold_mb) return false;
    if (battery_color_threshold_w != other.battery_color_threshold_w) return false;
    if (frequency_color_threshold_ghz != other.frequency_color_threshold_ghz) return false;
    if (auto_select_adapter != other.auto_select_adapter) return false;
    if (adapter_name != other.adapter_name) return false;
    if (check_updates != other.check_updates) return false;
    if (custom_label_colors != other.custom_label_colors) return false;
    if (custom_value_colors != other.custom_value_colors) return false;
    if (items.size() != other.items.size()) return false;
    for (const auto& [item, cfg] : items) {
        auto it = other.items.find(item);
        if (it == other.items.end()) return false;
        if (cfg.enabled != it->second.enabled || cfg.order != it->second.order ||
            cfg.custom_label != it->second.custom_label || cfg.font_size != it->second.font_size ||
            cfg.value_width != it->second.value_width || cfg.label_gap != it->second.label_gap ||
            cfg.offset_x != it->second.offset_x || cfg.show_unit != it->second.show_unit) return false;
    }
    return true;
}

} // namespace tmp
