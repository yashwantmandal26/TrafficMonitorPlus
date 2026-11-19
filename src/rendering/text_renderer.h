// TrafficMonitorPlus - High-level Text Renderer
#pragma once
#include "types.h"

namespace tmp {

class D2DRenderer;
class Theme;
struct Settings;

/// A prepared render item ready for drawing.
struct RenderItem {
    DisplayItem   item{};
    std::wstring  label;
    std::wstring  value;
    D2D1_COLOR_F  label_color{};
    D2D1_COLOR_F  value_color{};
    float         width = 0.0f;  // total measured width (label + label_gap + value)
    float label_width = 0, value_width = 0, height = 0, font_size = 0;
    float label_gap = 0, offset_x = 0;
    bool show_unit = true;
};

/// High-level text renderer that formats telemetry data with proper colors
/// and draws them via D2DRenderer.
class TextRenderer {
public:
    TextRenderer(D2DRenderer* renderer, Theme* theme);
    ~TextRenderer() = default;

    void SetRenderer(D2DRenderer* renderer) { m_renderer = renderer; }
    void SetTheme(Theme* theme) { m_theme = theme; }
    void SetThresholds(const Settings& settings);
    void SetCompactSpeed(bool compact) { m_compact_speed = compact; }
    void SetSpeedFormat(bool compact, bool bits, std::wstring unit) {
        m_compact_speed = compact; m_speed_bits = bits; m_speed_unit = std::move(unit);
    }
    void SetShowUnitSigns(bool show) { m_show_unit_signs = show; }
    void SetStatValueGap(int gap) { m_stat_value_gap = gap; }
    bool ShowUnitSigns() const { return m_show_unit_signs; }
    int StatValueGap() const { return m_stat_value_gap; }

    // Non-copyable
    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    /// Convert telemetry data into a list of renderable items.
    /// @param data            Current telemetry snapshot.
    /// @param enabled_items   Ordered list of (DisplayItem, DisplayItemConfig) pairs.
    /// @param thermal_colors  If true, apply thermal gradient coloring based on heat factor.
    std::vector<RenderItem> PrepareItems(
        const TelemetryData& data,
        const std::vector<std::pair<DisplayItem, DisplayItemConfig>>& enabled_items,
        bool thermal_colors,
        const std::map<DisplayItem, COLORREF>* label_colors = nullptr,
        const std::map<DisplayItem, COLORREF>* value_colors = nullptr);

    /// Draw items horizontally (left to right). Returns total width used.
    float RenderHorizontal(const std::vector<RenderItem>& items, float x, float y, float spacing);

    /// Draw items vertically (top to bottom). Returns total height used.
    float RenderVertical(const std::vector<RenderItem>& items, float x, float y, float spacing);

    /// Draw items in 2 rows across multiple columns (classic TrafficMonitor style). Returns total width used.
    float RenderTwoRows(const std::vector<RenderItem>& items, float start_x, float start_y, float col_spacing, float row_height);
    float RenderStacked(const std::vector<RenderItem>& items,float x,float y,float spacing,float row_height);
    float RenderAutoCompact(const std::vector<RenderItem>& items,float x,float y,float spacing,float available_height,float row_height);

private:
    void DrawItem(const RenderItem& item, float x, float y, float label_width);
    std::wstring m_family;
    float m_base_size = 13;
    bool m_bold = true;
    double m_cpu_limit=90,m_ram_limit=95,m_gpu_limit=90,m_temp_limit=90,m_download_limit=10*1024.0*1024,m_upload_limit=4*1024.0*1024,m_battery_limit=65,m_frequency_limit=5;
    /// Format a telemetry value for a given display item.
    std::wstring FormatValue(DisplayItem item, const TelemetryData& data, bool show_unit = true);

    /// Get the heat factor for a display item (for thermal coloring).
    float GetHeatFactor(DisplayItem item, const TelemetryData& data);

    D2DRenderer* m_renderer = nullptr;
    Theme*       m_theme    = nullptr;
    bool         m_compact_speed = true;
    bool         m_speed_bits = false;
    std::wstring m_speed_unit = L"auto";
    bool         m_show_unit_signs = true;
    int          m_stat_value_gap = 0;
};

} // namespace tmp
