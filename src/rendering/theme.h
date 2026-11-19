// TrafficMonitorPlus - Theme & Color Palette
#pragma once
#include "types.h"

namespace tmp {

/// Linear interpolation between two D2D colors.
D2D1_COLOR_F LerpColor(D2D1_COLOR_F a, D2D1_COLOR_F b, float t);

/// Theme manages the color palette for dark and light modes,
/// including thermal gradient and network quality colors.
class Theme {
public:
    Theme() = default;

    /// Initialize the theme palette.
    void Init(bool is_dark);

    /// Switch between dark and light mode.
    void SetDarkMode(bool dark);

    /// Returns true if currently in dark mode.
    bool IsDark() const;

    /// Default text color (white for dark, dark gray for light).
    D2D1_COLOR_F GetTextColor() const;

    /// Default label color (white on dark backgrounds).
    D2D1_COLOR_F GetLabelColor() const;

    /// Background color.
    D2D1_COLOR_F GetBackgroundColor() const;

    /// Accent color for highlights.
    D2D1_COLOR_F GetAccentColor() const;

    /// Returns a thermal-gradient color based on heat_factor [0.0, 1.0].
    /// Interpolates: white → yellow (#FFD83A) → orange (#FF8C00) → orange-red (#FF4500) → red (#FF1744).
    D2D1_COLOR_F GetThermalColor(float heat_factor) const;

    /// Returns a quality indicator color for network ping.
    D2D1_COLOR_F GetPingQualityColor(NetQuality quality) const;

    /// Convert COLORREF to D2D1_COLOR_F.
    static D2D1_COLOR_F ColorRefToD2D(COLORREF cr, float alpha = 1.0f);

    /// Convert D2D1_COLOR_F to COLORREF (alpha is discarded).
    static COLORREF D2DToColorRef(D2D1_COLOR_F color);

private:
    void BuildPalette();

    bool m_isDark = true;

    // Core palette
    D2D1_COLOR_F m_textColor{};
    D2D1_COLOR_F m_labelColor{};
    D2D1_COLOR_F m_bgColor{};
    D2D1_COLOR_F m_accentColor{};

    // Thermal gradient stops (5 entries: cool → warm → hot → very-hot → critical)
    static constexpr int THERMAL_STOPS = 5;
    D2D1_COLOR_F m_thermalStops[THERMAL_STOPS]{};
};

} // namespace tmp
