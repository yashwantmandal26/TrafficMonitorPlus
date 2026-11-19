// TrafficMonitorPlus - High-level Text Renderer Implementation
#include "pch.h"
#include "rendering/text_renderer.h"
#include "rendering/d2d_renderer.h"
#include "rendering/theme.h"
#include "utils/string_utils.h"
#include "config/settings.h"

namespace tmp {

// ─── Local string formatting helpers ──────────────────────────
// Self-contained formatting to avoid cross-module dependencies.

namespace {

std::wstring FormatTrafficSpeed(double bytes_per_sec, bool compact, bool bits, const std::wstring& unit) {
    if(!std::isfinite(bytes_per_sec)||bytes_per_sec<0)return L"--";
    if (!bits && unit == L"auto") return compact ? FormatSpeedCompact(bytes_per_sec) : FormatSpeed(bytes_per_sec);
    double value = bytes_per_sec * (bits ? 8.0 : 1.0);
    const bool fixed_mb = unit == L"MB/s";
    const bool fixed_kb = unit == L"KB/s";
    double scale = 1.0;
    const wchar_t* suffix = bits ? L"b/s" : L"B/s";
    if (fixed_mb) { scale = 1024.0 * 1024.0; suffix = bits ? L"Mb/s" : L"MB/s"; }
    else if (fixed_kb) { scale = 1024.0; suffix = bits ? L"Kb/s" : L"KB/s"; }
    else if (value >= 1024.0 * 1024.0 * 1024.0) { scale = 1024.0 * 1024.0 * 1024.0; suffix = bits ? L"Gb/s" : L"GB/s"; }
    else if (value >= 1024.0 * 1024.0) { scale = 1024.0 * 1024.0; suffix = bits ? L"Mb/s" : L"MB/s"; }
    else if (value >= 1024.0) { scale = 1024.0; suffix = bits ? L"Kb/s" : L"KB/s"; }
    if (compact) return std::format(L"{:.1f}{}", value / scale, suffix);
    return std::format(L"{:.1f} {}", value / scale, suffix);
}

/// Format display refresh rate. Example: "144 Hz"
std::wstring FormatHz(int hz) {
    if (hz <= 0) return L"-- Hz";
    return std::format(L"{} Hz", hz);
}

} // anonymous namespace

// ─── TextRenderer ─────────────────────────────────────────────

TextRenderer::TextRenderer(D2DRenderer* renderer, Theme* theme)
    : m_renderer(renderer)
    , m_theme(theme) {
}

std::wstring TextRenderer::FormatValue(DisplayItem item, const TelemetryData& data, bool show_unit) {
    auto formatSpeedNoUnit = [&](double bytes_per_sec) -> std::wstring {
        if (!std::isfinite(bytes_per_sec) || bytes_per_sec < 0) return L"--";
        double value = bytes_per_sec * (m_speed_bits ? 8.0 : 1.0);
        double scale = 1.0;
        if (m_speed_unit == L"MB/s") scale = 1024.0 * 1024.0;
        else if (m_speed_unit == L"KB/s") scale = 1024.0;
        else if (value >= 1024.0 * 1024.0 * 1024.0) scale = 1024.0 * 1024.0 * 1024.0;
        else if (value >= 1024.0 * 1024.0) scale = 1024.0 * 1024.0;
        else if (value >= 1024.0) scale = 1024.0;
        return std::format(L"{:.1f}", value / scale);
    };
    switch (item) {
        case DisplayItem::DownloadSpeed:
            if (!show_unit) return formatSpeedNoUnit(data.download_bytes_per_sec);
            return FormatTrafficSpeed(data.download_bytes_per_sec, m_compact_speed, m_speed_bits, m_speed_unit);
        case DisplayItem::UploadSpeed:
            if (!show_unit) return formatSpeedNoUnit(data.upload_bytes_per_sec);
            return FormatTrafficSpeed(data.upload_bytes_per_sec, m_compact_speed, m_speed_bits, m_speed_unit);
        case DisplayItem::CpuUsage:
            if (!std::isfinite(data.cpu_usage_percent) || data.cpu_usage_percent < 0.0) return L"--";
            return show_unit
                ? FormatPercent(data.cpu_usage_percent)
                : std::format(L"{}", std::lround(std::clamp(data.cpu_usage_percent, 0.0, 100.0)));
        case DisplayItem::CpuTemp:
            if (!std::isfinite(data.cpu_temp_celsius) || data.cpu_temp_celsius < 0.0) return L"--";
            if (!show_unit) {
                if (std::abs(data.cpu_temp_celsius - std::round(data.cpu_temp_celsius)) > 0.0001)
                    return std::format(L"{:.1f}", data.cpu_temp_celsius);
                return std::format(L"{:.0f}", data.cpu_temp_celsius);
            }
            return FormatTemperature(data.cpu_temp_celsius);
        case DisplayItem::RamUsage:
            if (!std::isfinite(data.ram_usage_percent) || data.ram_usage_percent < 0.0) return L"--";
            return show_unit
                ? FormatPercent(data.ram_usage_percent)
                : std::format(L"{}", std::lround(std::clamp(data.ram_usage_percent, 0.0, 100.0)));
        case DisplayItem::GpuUsage:
            if (!std::isfinite(data.gpu_usage_percent) || data.gpu_usage_percent < 0.0) return L"--";
            return show_unit
                ? FormatPercent(data.gpu_usage_percent)
                : std::format(L"{}", std::lround(std::clamp(data.gpu_usage_percent, 0.0, 100.0)));
        case DisplayItem::GpuTemp:
            if (!std::isfinite(data.gpu_temp_celsius) || data.gpu_temp_celsius < 0.0) return L"--";
            if (!show_unit) {
                if (std::abs(data.gpu_temp_celsius - std::round(data.gpu_temp_celsius)) > 0.0001)
                    return std::format(L"{:.1f}", data.gpu_temp_celsius);
                return std::format(L"{:.0f}", data.gpu_temp_celsius);
            }
            return FormatTemperature(data.gpu_temp_celsius);
        case DisplayItem::BatteryWattage:
            if (!std::isfinite(data.battery_wattage)) return L"--";
            return show_unit
                ? FormatWattage(data.battery_wattage)
                : std::format(L"{:.1f}", data.battery_wattage);
        case DisplayItem::CpuFrequency:
            if (!std::isfinite(data.cpu_freq_ghz) || data.cpu_freq_ghz < 0.0) return L"--";
            return show_unit
                ? FormatFrequency(data.cpu_freq_ghz)
                : std::format(L"{:.1f}", data.cpu_freq_ghz);
        default:                          return L"--";
    }
}

void TextRenderer::SetThresholds(const Settings& settings) {
    m_cpu_limit=settings.cpu_alert_threshold;m_ram_limit=settings.ram_alert_threshold;
    m_gpu_limit=settings.gpu_color_threshold;m_temp_limit=settings.temp_alert_threshold;
    m_download_limit=settings.download_color_threshold_mb*1024.0*1024;m_upload_limit=settings.upload_color_threshold_mb*1024.0*1024;
    m_battery_limit=settings.battery_color_threshold_w;m_frequency_limit=settings.frequency_color_threshold_ghz;
}
float TextRenderer::GetHeatFactor(DisplayItem item, const TelemetryData& data) {
    double value=-1,limit=1;
    switch (item) {
        case DisplayItem::DownloadSpeed:value=data.download_bytes_per_sec;limit=m_download_limit;break;
        case DisplayItem::UploadSpeed:value=data.upload_bytes_per_sec;limit=m_upload_limit;break;
        case DisplayItem::CpuUsage:value=data.cpu_usage_percent;limit=m_cpu_limit;break;
        case DisplayItem::RamUsage:value=data.ram_usage_percent;limit=m_ram_limit;break;
        case DisplayItem::GpuUsage:value=data.gpu_usage_percent;limit=m_gpu_limit;break;
        case DisplayItem::CpuTemp:value=data.cpu_temp_celsius;limit=m_temp_limit;break;
        case DisplayItem::GpuTemp:value=data.gpu_temp_celsius;limit=m_temp_limit;break;
        case DisplayItem::BatteryWattage:value=std::abs(data.battery_wattage);limit=m_battery_limit;break;
        case DisplayItem::CpuFrequency:value=data.cpu_freq_ghz;limit=m_frequency_limit;break;
        default:return -1;
    }
    if(value<0||!std::isfinite(value))return -1;
    limit=(std::max)(1.0,limit);
    if(item==DisplayItem::DownloadSpeed||item==DisplayItem::UploadSpeed){
        const double yellow=150.0*1024;
        const double orange=limit*(item==DisplayItem::DownloadSpeed?0.20:0.25);
        // Preserve the useful network landmarks, interpolating between them.
        if(value<yellow)return static_cast<float>(0.25*value/yellow);
        if(value<orange)return static_cast<float>(0.25+0.25*(value-yellow)/(orange-yellow));
        return static_cast<float>(std::clamp(0.5+0.5*(value-orange)/(limit-orange),0.5,1.0));
    }
    const double ratio=std::clamp(value/limit,0.0,1.0);
    // White at zero, yellow at half the limit, orange at three quarters,
    // then increasingly red up to the configured limit. No discrete bands.
    return static_cast<float>(ratio<0.5?ratio*0.5:ratio<0.75?ratio-0.25:ratio*2-1);
}

std::vector<RenderItem> TextRenderer::PrepareItems(
    const TelemetryData& data,
    const std::vector<std::pair<DisplayItem, DisplayItemConfig>>& enabled_items,
    bool thermal_colors,
    const std::map<DisplayItem, COLORREF>* label_colors,
    const std::map<DisplayItem, COLORREF>* value_colors) {

    std::vector<RenderItem> result;
    result.reserve(enabled_items.size());

    if (!m_renderer || !m_theme) return result;

    m_family = m_renderer->FontFamily();
    m_base_size = m_renderer->FontSize();
    m_bold = m_renderer->FontBold();
    for (const auto& [item, config] : enabled_items) {
        if (!config.enabled) continue;

        RenderItem ri{};
        ri.item = item;
        ri.font_size = config.font_size ? static_cast<float>(config.font_size) : m_base_size;
        m_renderer->SetFont(m_family, ri.font_size, m_bold);

        // Determine label text
        if (!config.custom_label.empty()) {
            ri.label = config.custom_label;
        } else if (item == DisplayItem::GpuUsage && !data.gpu_prefix.empty()) {
            ri.label = data.gpu_prefix;
        } else {
            const wchar_t* defaultLabel = GetDefaultLabel(item);
            if (defaultLabel && defaultLabel[0] != L'\0') {
                ri.label = defaultLabel;
            }
        }

        // Format the value
        const bool item_show_unit = config.show_unit && m_show_unit_signs;
        ri.show_unit = item_show_unit;
        ri.value = FormatValue(item, data, item_show_unit);

        // Colors
        ri.label_color = m_theme->GetLabelColor();
        if (label_colors) {
            auto it = label_colors->find(item);
            if (it != label_colors->end()) {
                ri.label_color = Theme::ColorRefToD2D(it->second);
            }
        }

        if (thermal_colors) {
            float heat = GetHeatFactor(item, data);
            if (heat >= 0.0f) {
                ri.value_color = m_theme->GetThermalColor(heat);
            } else {
                ri.value_color = m_theme->GetTextColor();
            }
        } else {
            ri.value_color = m_theme->GetTextColor();
        }

        if (value_colors && !thermal_colors) {
            auto it = value_colors->find(item);
            if (it != value_colors->end()) ri.value_color = Theme::ColorRefToD2D(it->second);
        }

        ri.label_gap = config.label_gap > 0 ? static_cast<float>(config.label_gap) : static_cast<float>(m_stat_value_gap);
        ri.offset_x = static_cast<float>(config.offset_x);

        // Slots depend only on configuration/font, never on live values.
        const auto label = item == DisplayItem::GpuUsage && config.custom_label.empty() ? L"dGPU:" : ri.label;
        ri.label_width = label.empty() ? 0 : m_renderer->MeasureText(label).width + 2;
        const wchar_t* sample = item_show_unit ? L"100%" : L"100";
        switch (item) {
            case DisplayItem::DownloadSpeed: case DisplayItem::UploadSpeed: sample = item_show_unit ? (m_compact_speed&&!m_speed_bits&&m_speed_unit==L"auto"?L"999.9M/s":L"999.9GB/s") : L"999.9"; break;
            case DisplayItem::CpuTemp: case DisplayItem::GpuTemp: sample = item_show_unit ? L"100.0°C" : L"100.0"; break;
            case DisplayItem::BatteryWattage: sample = item_show_unit ? L"-999.9W" : L"-999.9"; break;
            case DisplayItem::CpuFrequency: sample = item_show_unit ? L"9.99GHz" : L"9.99"; break;
        }
        ri.value_width = config.value_width ? static_cast<float>(config.value_width) : std::ceil(m_renderer->MeasureText(sample).width) + 2;
        ri.width = ri.label_width + (ri.label.empty() ? 0.0f : ri.label_gap) + ri.value_width;
        ri.height = m_renderer->MeasureText(L"Ag").height;

        result.push_back(std::move(ri));
    }

    m_renderer->SetFont(m_family, m_base_size, m_bold);
    return result;
}

void TextRenderer::DrawItem(const RenderItem& item, float x, float y, float label_width) {
    m_renderer->SetFont(m_family, item.font_size, m_bold);
    const float draw_x = x + item.offset_x;
    if (!item.label.empty()) m_renderer->DrawText(item.label, draw_x, y, item.label_color);
    const float val_x = draw_x + label_width + (item.label.empty() ? 0.0f : item.label_gap);
    m_renderer->DrawText(item.value, D2D1::RectF(val_x, y,
        val_x + item.value_width, y + item.height + 2), item.value_color);
    m_renderer->SetFont(m_family, m_base_size, m_bold);
}

float TextRenderer::RenderHorizontal(const std::vector<RenderItem>& items, float x, float y, float spacing) {
    if (!m_renderer) return 0;
    float current = x, end = x;
    for (const auto& item : items) {
        DrawItem(item, current, y, item.label_width);
        const float item_span = item.offset_x + item.width;
        end = (std::max)(end, current + item_span);
        current += (std::max)(1.0f, item_span + spacing);
    }
    return end - x;
}
float TextRenderer::RenderVertical(const std::vector<RenderItem>& items, float x, float y, float spacing) {
    if (!m_renderer) return 0;
    float current = y, end = y;
    for (const auto& item : items) {
        DrawItem(item, x, current, item.label_width);
        end = (std::max)(end, current + item.height + 2);
        current += (std::max)(1.0f, item.height + 2 + spacing);
    }
    return end - y;
}
float TextRenderer::RenderTwoRows(const std::vector<RenderItem>& items, float x, float y, float spacing, float row_height) {
    if (!m_renderer) return 0;
    float current = x, end = x;
    for (size_t i = 0; i < items.size(); i += 2) {
        const auto& top = items[i];
        const auto* bottom = i + 1 < items.size() ? &items[i + 1] : nullptr;
        const float label = (std::max)(top.label_width, bottom ? bottom->label_width : 0);
        DrawItem(top, current, y, label);
        if (bottom) DrawItem(*bottom, current, y + row_height, label);
        const float top_col = top.offset_x + label + (top.label.empty() ? 0.0f : top.label_gap) + top.value_width;
        const float btm_col = bottom ? (bottom->offset_x + label + (bottom->label.empty() ? 0.0f : bottom->label_gap) + bottom->value_width) : 0.0f;
        const float column = (std::max)(top_col, btm_col);
        end = (std::max)(end, current + column);
        current += (std::max)(1.0f, column + spacing);
    }
    return end - x;
}
float TextRenderer::RenderStacked(const std::vector<RenderItem>& items,float x,float y,float spacing,float row_height){
    if(!m_renderer)return 0;
    float current=x,end=x;
    for(const auto& item:items){
        m_renderer->SetFont(m_family,item.font_size,m_bold);
        std::wstring label=item.label;
        if(label.empty())label=item.item==DisplayItem::CpuTemp?(item.show_unit?L"CPU \u00b0C":L"CPU"):item.item==DisplayItem::GpuTemp?(item.show_unit?L"GPU \u00b0C":L"GPU"):L"";
        if(!label.empty()&&label.back()==L':')label.pop_back();
        const auto measured=item.item==DisplayItem::GpuUsage? (std::max)(m_renderer->MeasureText(label).width,m_renderer->MeasureText(L"dGPU").width):m_renderer->MeasureText(label).width;
        const float width=(std::max)(std::ceil(measured)+2,item.value_width);
        const float draw_x = current + item.offset_x;
        m_renderer->DrawText(label,draw_x+(width-m_renderer->MeasureText(label).width)/2,y,item.label_color);
        const float value_x=draw_x+(std::max)(0.0f,(width-m_renderer->MeasureText(item.value).width)/2);
        m_renderer->DrawText(item.value,D2D1::RectF(value_x,y+row_height,draw_x+width,y+row_height+item.height+2),item.value_color);
        end=(std::max)(end,draw_x+width);current+=(std::max)(1.0f,item.offset_x+width+spacing);
    }
    m_renderer->SetFont(m_family,m_base_size,m_bold);return end-x;
}
float TextRenderer::RenderAutoCompact(const std::vector<RenderItem>& items,float x,float y,float spacing,float available_height,float row_height){
    if(!m_renderer||items.empty())return 0;
    const bool two_rows=available_height+0.01f>=row_height*2;
    // Fixed row meaning: network first, then CPU/GPU usage, then CPU/GPU
    // temperature. CPU always occupies the upper device row.
    std::vector<std::vector<RenderItem>> groups;
    std::vector<int> keys;
    for(const auto& item:items){
        int key=100+static_cast<int>(item.item);
        switch(item.item){
            case DisplayItem::UploadSpeed:case DisplayItem::DownloadSpeed:key=0;break;
            case DisplayItem::CpuUsage:case DisplayItem::GpuUsage:key=1;break;
            case DisplayItem::CpuTemp:case DisplayItem::GpuTemp:key=2;break;
            default:break;
        }
        auto found=std::find(keys.begin(),keys.end(),key);
        if(found==keys.end()){keys.push_back(key);groups.push_back({item});}
        else groups[static_cast<size_t>(found-keys.begin())].push_back(item);
    }
    // Nonnegative spacing always includes a small readable gutter. Negative
    // spacing remains an explicit opt-in to overlapping groups.
    const float gap=spacing<0?spacing:(std::max)(3.0f,spacing);
    std::vector<size_t> order;
    for(size_t i=0;i<groups.size();++i)order.push_back(i);
    std::stable_sort(order.begin(),order.end(),[&](size_t a,size_t b){
        return (keys[a]<3?keys[a]:3)<(keys[b]<3?keys[b]:3);
    });
    float current=x,end=x;
    for(auto index:order){
        auto group=groups[index];
        std::stable_sort(group.begin(),group.end(),[](const RenderItem& a,const RenderItem& b){
            const bool at=a.item==DisplayItem::DownloadSpeed||a.item==DisplayItem::GpuUsage||a.item==DisplayItem::GpuTemp;
            const bool bt=b.item==DisplayItem::DownloadSpeed||b.item==DisplayItem::GpuUsage||b.item==DisplayItem::GpuTemp;
            return !at&&bt;
        });

        if(group.size()==1&&group[0].label.empty()){
            auto& item=group[0];
            item.label=item.item==DisplayItem::CpuTemp?(item.show_unit?L"CPU \u00b0C":L"CPU"):item.item==DisplayItem::GpuTemp?(item.show_unit?L"GPU \u00b0C":L"GPU"):L"";
            m_renderer->SetFont(m_family,item.font_size,m_bold);
            item.label_width=m_renderer->MeasureText(item.label).width+2;
            item.width=item.label_width+(item.label.empty()?0.0f:item.label_gap)+item.value_width;
            m_renderer->SetFont(m_family,m_base_size,m_bold);
        }
        float width=0;
        if(two_rows&&group.size()>1){
            float label=0;
            for(const auto& item:group){
                label=(std::max)(label,item.label_width);
            }
            float max_extent=0;
            for(const auto& item:group){
                const float eff_val_w = (std::max)({item.value_width, std::ceil(m_renderer->MeasureText(item.value).width) + 1.0f});
                const float row_extent = item.offset_x + label + (item.label.empty() ? 0.0f : item.label_gap) + eff_val_w;
                max_extent = (std::max)(max_extent, row_extent);
            }
            width=max_extent;
            for(size_t row=0;row<group.size();++row)
                DrawItem(group[row],current,y+row*row_height,label);
        }else if(two_rows&&group.size()==1){
            const auto& item=group[0];
            m_renderer->SetFont(m_family,item.font_size,m_bold);
            std::wstring label=item.label;if(!label.empty()&&label.back()==L':')label.pop_back();
            float label_width=m_renderer->MeasureText(label).width;
            if(item.item==DisplayItem::GpuUsage)label_width=(std::max)(label_width,m_renderer->MeasureText(L"dGPU").width);
            m_renderer->SetFont(m_family,m_base_size,m_bold);
            const float stacked_width=(std::max)(std::ceil(label_width)+2,item.value_width);
            if(stacked_width<item.width)width=RenderStacked(group,current,y,0,row_height);
            else{DrawItem(item,current,y,item.label_width);width=item.offset_x+item.width;}
        }else width=RenderHorizontal(group,current,y,gap);
        end=(std::max)(end,current+width);current+=(std::max)(1.0f,width+gap);
    }
    return end-x;
}
} // namespace tmp

