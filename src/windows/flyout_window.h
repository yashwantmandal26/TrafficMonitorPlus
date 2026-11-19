// TrafficMonitorPlus - Interactive Quick Panel Dashboard Window
#pragma once
#include "pch.h"
#include "types.h"
#include "rendering/d2d_renderer.h"
#include "rendering/theme.h"
#include "rendering/sparkline.h"
#include "telemetry/process_monitor.h"
#include "telemetry/windows_network_usage.h"
#include "telemetry/live_network_types.h"
#include <future>

namespace tmp {

struct DiskInfo {
    wchar_t letter = 0;
    std::wstring label;
    uint64_t total = 0;
    uint64_t free = 0;
    uint64_t used = 0;
    double used_pct = 0.0;
};

struct KillTarget {
    std::wstring name;
    std::wstring display_name;
    DWORD pid = 0;
    bool is_service = false;
    std::wstring service_name;
    uint64_t created = 0;
    std::wstring executable_path;
};

class FlyoutWindow {
public:
    FlyoutWindow();
    ~FlyoutWindow();

    bool Init(HINSTANCE hInstance);
    void Show(POINT anchor, bool above_anchor = true);
    void Hide();
    void Toggle(POINT anchor, bool above_anchor = true);
    bool IsVisible() const;
    HWND GetHwnd() const { return m_hwnd; }
    void SetPinned(bool pinned) { m_pinned = pinned; if (m_hwnd) InvalidateRect(m_hwnd, nullptr, FALSE); }

    void UpdateData(const TelemetryData& data, const std::vector<ProcessInfo>& top_processes, const std::wstring& local_ip = L"");
    void UpdateNetworkReport(const NetworkUsageReport& report);
    void UpdateLiveNetwork(const LiveNetworkSnapshot& snapshot);
    void RefreshNetworkUsage();
    void RefreshStorageSnapshot();
    void RefreshStorageAsync();

    // Callbacks for action buttons
    std::function<bool()> on_copy_ip;
    std::function<void()> on_data_usage;
    std::function<void()> on_open_task_manager;
    std::function<void()> on_open_settings;
    std::function<void(wchar_t)> on_open_drive;
    std::function<void()> on_dismissed;
    std::function<void()> on_shown;
    std::function<void()> on_retry_live_network;
    std::function<bool(const std::wstring& name, DWORD pid, bool is_service)> on_kill_target;

    // Seams for testing / decoupling
    std::function<std::vector<DiskInfo>()> storage_provider;
    std::function<std::wstring()> ip_provider;
    std::function<NetworkUsageReport(bool force)> network_usage_provider;

    // Direct termination action helper
    bool TerminateTarget(const KillTarget& target, std::wstring& out_message);
    bool IsConfirmingKill() const { return m_confirm_active; }
    const KillTarget& GetPendingKill() const { return m_pending_kill; }

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

private:
    friend struct FlyoutWindowTestAccess;
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    void Render();
    void NormalizePage();
    int ContentExtent() const;
    int MaxScroll() const;
    void DragScrollbar(float y);
    void PrepareNetworkClose(const std::wstring& id);
    void OnLButtonUp(int x, int y);
    float CanvasScale() const;
    int ControlAt(float x, float y) const;
    void ActivateControl(int control);
    void ChangePage(int page);
    void Scroll(int direction);
    void AcceptStorageSnapshot();
    std::vector<DiskInfo> SampleDrives();

    HWND      m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    bool      m_registered = false;

    int m_width = 480;
    int m_height = 632;
    int m_hover = -1, m_focus = -1, m_page = 0, m_scroll = 0;
    int m_quick_filter = 0; // 0 = All Hogs, 1 = Data Hogs, 2 = System Hogs
    bool m_sort_memory = false, m_pinned = false, m_has_data = false;
    bool m_tracking = false;
    bool m_scroll_dragging = false;
    float m_scroll_grab = 0;
    int m_wheel_remainder = 0;
    std::wstring m_notice;

    // Confirmation dialog state for powerful 'x' kill
    bool m_confirm_active = false;
    KillTarget m_pending_kill{};
    std::vector<KillTarget> m_pending_targets, m_service_options;
    bool m_service_picker = false;
    int m_service_page = 0;

    std::unique_ptr<D2DRenderer> m_renderer;
    Theme m_theme;

    // Sparklines for live 60s history
    Sparkline m_net_sparkline;
    Sparkline m_upload_sparkline;
    Sparkline m_cpu_sparkline;
    Sparkline m_ram_sparkline;

    TelemetryData            m_latest_data{};
    std::vector<ProcessInfo> m_top_processes;
    NetworkUsageReport       m_cached_net_usage{};
    LiveNetworkSnapshot      m_live_network{};

    // Cached storage & network snapshots (decoupled from paint path)
    std::vector<DiskInfo> m_cached_drives;
    std::wstring m_cached_ip = L"0.0.0.0";
    std::chrono::steady_clock::time_point m_last_storage_refresh{};
    std::chrono::steady_clock::time_point m_last_network_refresh{};
    bool m_storage_refreshing = false;
    std::future<std::vector<DiskInfo>> m_storage_worker;

    // Hit-test rectangles for clickable buttons
    struct Control { D2D1_RECT_F rect; int id; };
    std::vector<Control> m_controls;

    // Action mappings for row kill buttons
    struct KillAction {
        int control_id;
        KillTarget target;
    };
    std::vector<KillAction> m_kill_actions;
};

} // namespace tmp
