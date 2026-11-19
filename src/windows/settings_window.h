#pragma once
#include "pch.h"
#include "types.h"
#include "config/settings.h"
#include "telemetry/usage_history.h"
#include "telemetry/windows_network_usage.h"
#include "rendering/d2d_renderer.h"
#include "rendering/text_renderer.h"
#include "rendering/theme.h"

namespace tmp {
class SettingsWindow {
public:
    SettingsWindow();
    ~SettingsWindow();
    bool Init(HINSTANCE instance);
    void Show(const Settings& settings);
    void Hide();
    bool IsVisible() const;
    bool HasUnsavedChanges() const;
    void Discard();
    HWND GetHwnd() const { return m_hwnd; }
    int CurrentPage() const { return m_page; }
    void RefreshReadouts() { if (IsVisible() && (m_page==0 || m_page==4)) InvalidateRect(m_hwnd,nullptr,FALSE); }
    std::function<bool(const Settings&)> on_settings_changed;
    std::function<void(const Settings&)> on_layout_preview;
    std::function<void()> on_preview_finished;
    std::function<Settings()> live_settings;
    std::function<TelemetryData()> data_snapshot;
    std::function<std::pair<UsageTotal,UsageTotal>()> usage_snapshot;
    std::function<NetworkUsageReport(UsageTimeRange,UsageNetworkScope,bool)> detailed_usage_snapshot;
    std::function<bool(const fs::path&)> on_export_usage;
    std::function<bool(const fs::path&,const NetworkUsageReport&)> on_export_detailed_usage;
    std::function<void()> on_open_history, on_sensor_status;
    std::function<void()> on_repair_startup;
    std::function<bool()> startup_status;
    std::function<std::wstring()> history_status;
    std::function<bool()> is_native_windows_usage;
private:
    static LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
    static LRESULT CALLBACK LabelProc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);
    void Render();
    void Activate(float x,float y);
    void Text(const std::wstring&,float x,float y,float width,float size=13,bool bold=false,D2D1_COLOR_F colour=D2D1::ColorF(0.85f,0.89f,0.95f));
    void TextRight(const std::wstring&,float right_x,float y,float max_width,float size=12,bool bold=false,D2D1_COLOR_F colour=D2D1::ColorF(0.85f,0.89f,0.95f));
    void TextCenter(const std::wstring&,float x,float y,float width,float size=11,bool bold=false,D2D1_COLOR_F colour=D2D1::ColorF(0.85f,0.89f,0.95f));
    std::wstring Ellipsize(const std::wstring&,float max_width,float size,bool bold);
    void Card(float x,float y,float w,float h);
    void Section(float y,float height,const std::wstring& title,const std::wstring& description=L"");
    void Navigation(int page,float y,const std::wstring& title);
    void Button(D2D1_RECT_F,const std::wstring&,std::function<void()>,bool selected=false);
    void PillButton(D2D1_RECT_F,const std::wstring&,std::function<void()>,bool selected=false,bool enabled=true,float font_size=11.0f);
    void Toggle(float y,const std::wstring&,const std::wstring&,bool&,float x=212,float width=584);
    void Checkbox(float x,float y,const std::wstring& title,bool& value,float width=270,std::function<void(bool)> on_change=nullptr);
    void Stepper(float x,float y,const std::wstring&,int&,int min,int max,int step=1,float width=270);
    void Page(int page);
    void SelectMetric(DisplayItem item);
    void MoveMetric(int direction);
    void RefreshOrder();
    void Save();
    void PositionLabel();
    void PositionSliders();
    float Scale() const;
    HWND m_hwnd=nullptr,m_label_edit=nullptr;
    HWND m_layout_sliders[3]{};
    HINSTANCE m_instance=nullptr;
    HFONT m_edit_font=nullptr;
    HBRUSH m_edit_brush=nullptr;
    int m_page=0,m_hover=-1,m_focus=-1;
    int m_layout_tab=0;
    int m_metric_tab=0;
    UsageTimeRange m_usage_range=UsageTimeRange::Today;
    UsageNetworkScope m_usage_scope=UsageNetworkScope::ActiveOnly;
    int m_usage_category_filter=0;
    int m_usage_scroll=0;
    int m_usage_max_scroll=0;
    int m_selected_bucket_idx=-1;
    int m_bucket_zone_start=-1;
    int m_bucket_zone_count=0;
    bool m_usage_force_refresh=false;
    std::wstring m_selected_app_id;
    NetworkUsageReport m_last_report{};
    std::wstring m_status;
    bool m_tracking=false,m_reset=false,m_startup_ready=false;
    DisplayItem m_metric=DisplayItem::UploadSpeed;
    std::vector<DisplayItem> m_order;
    struct Zone {D2D1_RECT_F rect;std::function<void()> action;};
    std::vector<Zone> m_zones;
    Settings m_draft{},m_original{};
    Theme m_theme;
    std::unique_ptr<D2DRenderer> m_renderer;
    std::unique_ptr<TextRenderer> m_preview;
};
}
