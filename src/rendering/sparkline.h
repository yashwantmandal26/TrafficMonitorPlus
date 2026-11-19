// TrafficMonitorPlus - 60-Second Real-Time Sparkline Graph
#pragma once
#include "pch.h"
#include "utils/ring_buffer.h"
#include "rendering/d2d_renderer.h"

namespace tmp {

class Sparkline {
public:
    static constexpr size_t SAMPLE_COUNT = 60; // 60 seconds of history

    Sparkline() = default;
    ~Sparkline() = default;

    void Push(double value) {
        auto now = std::chrono::steady_clock::now();
        if(std::isfinite(value)&&value>=0)m_samples.emplace_back(now, value);
        std::erase_if(m_samples, [now](const auto& sample) {
            return now - sample.first > std::chrono::seconds(60);
        });
    }

    void Clear() {
        m_samples.clear();
    }
    double Peak() const {
        const auto now=std::chrono::steady_clock::now();
        double peak=0;
        for(const auto& sample:m_samples)if(now-sample.first<=std::chrono::seconds(60))peak=(std::max)(peak,sample.second);
        return peak;
    }

    /// Renders the sparkline graph within the specified bounding rectangle.
    /// @param renderer     The Direct2D renderer
    /// @param rect         The drawing rectangle
    /// @param line_color   The stroke color of the graph line
    /// @param fill_color   The gradient/semi-transparent under-fill color
    /// @param fixed_max    If > 0, scales relative to this maximum (e.g. 100 for %);
    ///                     otherwise, auto-scales to the peak value in the 60s window.
    void Render(D2DRenderer* renderer, D2D1_RECT_F rect, D2D1_COLOR_F line_color, D2D1_COLOR_F fill_color, double fixed_max = -1.0) const;

private:
    std::vector<std::pair<std::chrono::steady_clock::time_point, double>> m_samples;
};

} // namespace tmp
