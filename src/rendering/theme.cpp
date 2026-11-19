// TrafficMonitorPlus - Theme Implementation
#include "pch.h"
#include "rendering/theme.h"

namespace tmp {

// ─── Free function ─────────────────────────────────────────
D2D1_COLOR_F LerpColor(D2D1_COLOR_F a, D2D1_COLOR_F b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return D2D1::ColorF(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    );
}

// ─── Helper: hex COLORREF to D2D color ─────────────────────
static D2D1_COLOR_F HexToD2D(uint32_t rgb, float alpha = 1.0f) {
    float r = static_cast<float>((rgb >> 16) & 0xFF) / 255.0f;
    float g = static_cast<float>((rgb >> 8)  & 0xFF) / 255.0f;
    float b = static_cast<float>((rgb)       & 0xFF) / 255.0f;
    return D2D1::ColorF(r, g, b, alpha);
}

// ─── Theme ─────────────────────────────────────────────────

void Theme::Init(bool is_dark) {
    m_isDark = is_dark;
    BuildPalette();
}

void Theme::SetDarkMode(bool dark) {
    if (m_isDark == dark) return;
    m_isDark = dark;
    BuildPalette();
}

bool Theme::IsDark() const {
    return m_isDark;
}

D2D1_COLOR_F Theme::GetTextColor() const {
    return m_textColor;
}

D2D1_COLOR_F Theme::GetLabelColor() const {
    return m_labelColor;
}

D2D1_COLOR_F Theme::GetBackgroundColor() const {
    return m_bgColor;
}

D2D1_COLOR_F Theme::GetAccentColor() const {
    return m_accentColor;
}

D2D1_COLOR_F Theme::GetThermalColor(float heat_factor) const {
    heat_factor = std::clamp(heat_factor, 0.0f, 1.0f);

    // Map heat_factor [0,1] to a position across THERMAL_STOPS-1 segments
    float scaled = heat_factor * static_cast<float>(THERMAL_STOPS - 1);
    int   idx    = static_cast<int>(scaled);

    // Clamp index to valid segment range
    if (idx >= THERMAL_STOPS - 1) {
        return m_thermalStops[THERMAL_STOPS - 1];
    }

    float frac = scaled - static_cast<float>(idx);

    // Smooth-step interpolation for more visually pleasing transitions
    float t = frac * frac * (3.0f - 2.0f * frac);

    return LerpColor(m_thermalStops[idx], m_thermalStops[idx + 1], t);
}

D2D1_COLOR_F Theme::GetPingQualityColor(NetQuality quality) const {
    switch (quality) {
        case NetQuality::Good:    return HexToD2D(0x4CAF50);   // Green
        case NetQuality::Fair:    return HexToD2D(0xFFC107);   // Yellow/Amber
        case NetQuality::Poor:    return HexToD2D(0xF44336);   // Red
        case NetQuality::Unknown:
        default:                  return HexToD2D(0x9E9E9E);   // Gray
    }
}

D2D1_COLOR_F Theme::ColorRefToD2D(COLORREF cr, float alpha) {
    float r = static_cast<float>(GetRValue(cr)) / 255.0f;
    float g = static_cast<float>(GetGValue(cr)) / 255.0f;
    float b = static_cast<float>(GetBValue(cr)) / 255.0f;
    return D2D1::ColorF(r, g, b, alpha);
}

COLORREF Theme::D2DToColorRef(D2D1_COLOR_F color) {
    auto toByte = [](float v) -> BYTE {
        return static_cast<BYTE>(std::clamp(v * 255.0f, 0.0f, 255.0f));
    };
    return RGB(toByte(color.r), toByte(color.g), toByte(color.b));
}

void Theme::BuildPalette() {
    if (m_isDark) {
        // Dark mode palette
        m_textColor   = D2D1::ColorF(0.96f, 0.96f, 0.96f, 1.0f);  // Crisp white
        m_labelColor  = HexToD2D(0xFFFFFF);                         // Neutral white labels and arrows
        m_bgColor     = D2D1::ColorF(0.0f,  0.0f,  0.0f,  0.0f);  // Transparent (taskbar blends)
        m_accentColor = HexToD2D(0xFFFFFF);                         // Neutral default accent

        // Values progress from normal white through yellow/orange to critical red.
        m_thermalStops[0] = HexToD2D(0xFFFFFF);  // 0.00 - normal usage
        m_thermalStops[1] = HexToD2D(0xFFD83A);  // 0.25 - warm yellow
        m_thermalStops[2] = HexToD2D(0xFF8C00);  // 0.50 - amber
        m_thermalStops[3] = HexToD2D(0xFF4500);  // 0.75 - orange-red
        m_thermalStops[4] = HexToD2D(0xFF1744);  // 1.00 - critical red
    } else {
        // Light mode palette
        m_textColor   = D2D1::ColorF(0.13f, 0.13f, 0.13f, 1.0f);  // ~#212121
        m_labelColor  = HexToD2D(0x1F2937);                         // Crisp dark slate (WCAG AA > 5:1 contrast on light)
        m_bgColor     = D2D1::ColorF(1.0f,  1.0f,  1.0f,  0.0f);  // Transparent
        m_accentColor = HexToD2D(0x005FB8);                         // Windows 11 light accent

        // In light mode, base stop must be dark to contrast with light backgrounds.
        m_thermalStops[0] = HexToD2D(0x212121);  // 0.00 - normal usage (dark text)
        m_thermalStops[1] = HexToD2D(0xB27B00);  // 0.25 - warm amber
        m_thermalStops[2] = HexToD2D(0xD35400);  // 0.50 - deep orange
        m_thermalStops[3] = HexToD2D(0xC0392B);  // 0.75 - strong red
        m_thermalStops[4] = HexToD2D(0xB71C1C);  // 1.00 - critical deep red
    }
}

} // namespace tmp
