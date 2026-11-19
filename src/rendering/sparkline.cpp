// TrafficMonitorPlus - 60-Second Real-Time Sparkline Graph Implementation
#include "pch.h"
#include "rendering/sparkline.h"

namespace tmp {

void Sparkline::Render(
    D2DRenderer* renderer,
    D2D1_RECT_F rect,
    D2D1_COLOR_F line_color,
    D2D1_COLOR_F fill_color,
    double fixed_max
) const {
    if (!renderer) return;
    auto* target = renderer->GetTarget();
    if (!target) return;

    auto snapshot = m_samples;
    auto now = std::chrono::steady_clock::now();
    if (snapshot.size() < 2) return;

    float width  = rect.right - rect.left;
    float height = rect.bottom - rect.top;
    if (width <= 4.0f || height <= 4.0f) return;

    // Determine scale max
    double max_val = fixed_max;
    if (max_val <= 0.0) {
        max_val = 1.0;
        for (const auto& sample : snapshot) {
            if (sample.second > max_val) max_val = sample.second;
        }
    }

    size_t count = snapshot.size();

    // Build points
    std::vector<D2D1_POINT_2F> points;
    points.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        float age = std::chrono::duration<float>(now - snapshot[i].first).count();
        float x = rect.right - std::clamp(age / 60.0f, 0.0f, 1.0f) * width;
        float norm = static_cast<float>(std::clamp(snapshot[i].second / max_val, 0.0, 1.0));
        float y = rect.bottom - (norm * (height - 2.0f)) - 1.0f;
        points.push_back(D2D1::Point2F(x, y));
    }

    // Direct2D Factory for Path Geometry
    ComPtr<ID2D1Factory> factory;
    target->GetFactory(&factory);
    if (!factory) return;

    ComPtr<ID2D1PathGeometry> path;
    if (FAILED(factory->CreatePathGeometry(&path))) return;

    ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(path->Open(&sink))) return;

    // Under-curve filled polygon
    sink->BeginFigure(D2D1::Point2F(points[0].x, rect.bottom), D2D1_FIGURE_BEGIN_FILLED);
    for (const auto& pt : points) {
        sink->AddLine(pt);
    }
    sink->AddLine(D2D1::Point2F(points.back().x, rect.bottom));
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    sink->Close();

    // Fill brush
    ComPtr<ID2D1SolidColorBrush> fillBrush;
    if (SUCCEEDED(target->CreateSolidColorBrush(fill_color, &fillBrush))) {
        target->FillGeometry(path.Get(), fillBrush.Get());
    }

    // Line stroke path
    ComPtr<ID2D1PathGeometry> linePath;
    if (SUCCEEDED(factory->CreatePathGeometry(&linePath))) {
        ComPtr<ID2D1GeometrySink> lineSink;
        if (SUCCEEDED(linePath->Open(&lineSink))) {
            lineSink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_HOLLOW);
            for (size_t i = 1; i < points.size(); ++i) {
                lineSink->AddLine(points[i]);
            }
            lineSink->EndFigure(D2D1_FIGURE_END_OPEN);
            lineSink->Close();

            ComPtr<ID2D1SolidColorBrush> lineBrush;
            if (SUCCEEDED(target->CreateSolidColorBrush(line_color, &lineBrush))) {
                target->DrawGeometry(linePath.Get(), lineBrush.Get(), 1.5f);
            }
        }
    }
}

} // namespace tmp
