// TrafficMonitorPlus - Application Controller
#pragma once
#include "pch.h"
#include "types.h"

// Subsystem headers
#include "config/settings.h"
#include "config/autostart.h"
#include "config/updater.h"
#include "rendering/theme.h"
#include "rendering/dpi_helper.h"
#include "rendering/d2d_renderer.h"
#include "rendering/text_renderer.h"
#include "telemetry/telemetry_engine.h"
#include "utils/win_version.h"
#include "utils/string_utils.h"
#include "windows/tray_icon.h"
#include "windows/taskbar_host.h"
#include "windows/flyout_window.h"
#include "windows/settings_window.h"
#include "telemetry/process_monitor.h"
#include "telemetry/usage_history.h"
#include "telemetry/windows_network_usage.h"
#include "telemetry/live_network_monitor.h"

namespace tmp {

struct WindowsUsageSnapshot {
    UsageTotal today{};
    UsageTotal month{};
    bool has_usage = false;
    bool has_today = false;
    bool has_month = false;
    std::chrono::system_clock::time_point timestamp{};
    std::wstring query_date;
    std::wstring query_month;
    bool is_stale = false;
    std::wstring error;
};

struct DetailedUsageCacheEntry {
    NetworkUsageReport report;
    std::chrono::steady_clock::time_point fetched_at{};
};

struct UsageWorkerState {
    std::atomic<bool> cancelled{false};
    std::atomic<HWND> target_hwnd{nullptr};
    std::atomic<int>  active_workers{0};
    std::atomic<bool> fetching_summary{false};
    std::mutex        detailed_mutex;
    std::set<std::pair<int, int>> detailed_inflight;
    std::map<std::pair<int, int>, DetailedUsageCacheEntry> detailed_cache;
};

/// Central application controller (singleton).
/// Owns all subsystems and orchestrates the polling + rendering loop.
class App {
public:
    /// Returns the singleton instance.
    static App& Instance();

    /// Initialize all subsystems. Returns true on success.
    bool Init(HINSTANCE hInstance);

    /// Run the Windows message loop. Returns exit code.
    int Run();

    /// Tear down all subsystems cleanly.
    void Shutdown();

    /// Called by the timer every poll_interval_ms.
    void OnTimer();
    // Shared settings/diagnostics directory (user profile or explicit portable mode).
    static fs::path GetConfigPath();
    static bool RequestRunningInstanceExit();
    static bool RequestRunningInstanceShow();
    static std::wstring GetSingleInstanceMutexName();

    /// Called by tray icon menu handlers.
    void OnMenuCommand(UINT cmd_id);

    /// Accessors
    Settings& GetSettings();
    TelemetryEngine& GetTelemetry();
    Theme& GetTheme();

    // Non-copyable, non-movable
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;

private:
    App() = default;
    ~App() = default;

    /// Creates hidden message window for timers/messages.
    bool CreateMessageWindow();
    void StartUpdateCheck();

    /// Window procedure for the hidden message window.
    static LRESULT CALLBACK MessageWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /// Render callback – called when taskbar host invalidates.
    void RenderTaskbar(HWND hwnd);

    /// Update the tray icon tooltip text with current stats.
    void UpdateTrayTooltip();

    /// Build and show the tray context menu.
    void ShowTrayMenu();
    void PreviewLayout(const Settings* draft);

    /// Copy text to the Windows clipboard.
    static bool CopyToClipboard(HWND owner, const std::wstring& text);

    /// Get the exe directory.
    static fs::path GetExeDir();

    HINSTANCE m_hInstance = nullptr;
    HWND      m_hMsgWnd  = nullptr;    // Hidden message window
    UINT_PTR  m_timer_id = 0;
    int       m_tick_count = 0;        // Counts timer ticks for tooltip throttle
    bool      m_menu_open = false;
    struct LayoutPreview { int x,y,spacing; };
    std::optional<LayoutPreview> m_layout_preview;
    std::optional<Settings> m_visual_preview;
    UsageHistory m_usage;
    std::chrono::steady_clock::time_point m_usage_saved{};
    mutable std::mutex m_usage_snapshot_mutex;
    WindowsUsageSnapshot m_win_usage_snapshot{};
    std::shared_ptr<UsageWorkerState> m_worker_state;
    std::atomic<bool> m_shutting_down{false};
    std::chrono::steady_clock::time_point m_last_win_usage_fetch{};
    int m_win_usage_backoff_sec = 3;

    WindowsUsageSnapshot GetUsageSnapshot() const;
    void RefreshWindowsUsageAsync();
    NetworkUsageReport GetOrRequestDetailedUsage(UsageTimeRange range, UsageNetworkScope scope, bool force_refresh);
    void RequestDetailedUsageAsync(UsageTimeRange range, UsageNetworkScope scope, bool force_refresh);
    std::chrono::steady_clock::time_point m_cpu_alert_since{};
    bool m_cpu_alert_sent = false;
    bool m_ram_alert_sent = false;
    bool m_temp_alert_sent = false;
    std::wstring m_update_notice;

    // Windows version info (cached at init)
    WinVersionInfo m_winver{};

    // Subsystems (unique_ptr for deterministic lifecycle)
    std::unique_ptr<Settings>        m_settings;
    std::unique_ptr<TelemetryEngine> m_telemetry;
    std::unique_ptr<Theme>           m_theme;
    std::unique_ptr<D2DRenderer>     m_taskbar_renderer;
    std::unique_ptr<TextRenderer>    m_text_renderer;
    std::unique_ptr<TrayIcon>        m_tray;
    std::unique_ptr<TaskbarHost>     m_taskbar;
    std::unique_ptr<FlyoutWindow>    m_flyout;
    std::unique_ptr<SettingsWindow>  m_settings_window;
    std::unique_ptr<ProcessMonitor>  m_process_monitor;
    std::unique_ptr<LiveNetworkMonitor> m_live_network;
    std::unique_ptr<Updater>         m_updater;

    // Window class name for hidden message window
    static constexpr const wchar_t* MSG_WND_CLASS = L"TrafficMonitorPlusMsg";
    static constexpr UINT TIMER_ID = 1;
    static constexpr int  HOTKEY_ID = 9001;
};

} // namespace tmp
