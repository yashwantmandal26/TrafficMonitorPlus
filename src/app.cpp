// TrafficMonitorPlus - Application Controller Implementation
#include "pch.h"
#include "app.h"
#include "config/cli_parser.h"
#include "telemetry/windows_network_usage.h"
#include "telemetry/usage_accounting.h"
#include <thread>

namespace tmp {

static std::wstring UsageTooltip(const WindowsUsageSnapshot& snapshot,const UsageTotal& fallback,const std::wstring& date){
    const bool use_native=PreferWindowsUsage(snapshot.has_today&&snapshot.query_date==date,snapshot.today,fallback);
    const auto total=use_native?snapshot.today:fallback;
    return std::format(L"Today ({})\nDownload: {}\nUpload: {}\nCombined: {}",date,
        FormatMemory(total.download),FormatMemory(total.upload),FormatMemory(total.Combined()));
}

// ────────────────────────────────────────────────────────
// Singleton
// ────────────────────────────────────────────────────────
App& App::Instance() {
    static App s_instance;
    return s_instance;
}

std::wstring App::GetSingleInstanceMutexName() {
    return tmp::GetSingleInstanceMutexName(AutoStart::GetCurrentUserSid());
}

bool App::RequestRunningInstanceShow() {
    HWND window=FindWindowW(MSG_WND_CLASS,nullptr);
    if(window){
        DWORD process=0;GetWindowThreadProcessId(window,&process);
        AllowSetForegroundWindow(process);
        return PostMessageW(window,RegisterWindowMessageW(SHOW_SETTINGS_MESSAGE),0,0)!=FALSE;
    }
    // Read-only mutex access also works when the running instance is elevated.
    const std::wstring mutex_name = GetSingleInstanceMutexName();
    HANDLE existing=OpenMutexW(SYNCHRONIZE,FALSE,mutex_name.c_str());
    const DWORD error=GetLastError();
    if(!existing&&error!=ERROR_ACCESS_DENIED)return false;
    if(existing)CloseHandle(existing);
    PostMessageW(HWND_BROADCAST,RegisterWindowMessageW(SHOW_SETTINGS_MESSAGE),0,0);
    return true;
}

bool App::RequestRunningInstanceExit() {
    HWND window=FindWindowW(MSG_WND_CLASS,nullptr);
    if(!window)return true;
    DWORD pid=0;GetWindowThreadProcessId(window,&pid);
    if(!pid||pid==GetCurrentProcessId())return false;

    DWORD targetSession = 0, currentSession = 0;
    ProcessIdToSessionId(pid, &targetSession);
    ProcessIdToSessionId(GetCurrentProcessId(), &currentSession);
    if (targetSession != currentSession) return false;

    HANDLE process=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
    if(!process)return false;
    std::vector<wchar_t> path(32768);DWORD size=static_cast<DWORD>(path.size());
    const bool verified=QueryFullProcessImageNameW(process,0,path.data(),&size) &&
        _wcsicmp(fs::path(std::wstring(path.data(),size)).filename().c_str(),L"TrafficMonitorPlus.exe")==0;
    const bool stopped=verified && PostMessageW(window,WM_COMMAND,IDM_EXIT,0) &&
        WaitForSingleObject(process,20000)==WAIT_OBJECT_0;
    CloseHandle(process);
    return stopped;
}

// ────────────────────────────────────────────────────────
// File / Path Helpers
// ────────────────────────────────────────────────────────
fs::path App::GetExeDir() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return fs::path(path).parent_path();
}

fs::path App::GetConfigPath() {
    // Explicit portable mode; normal installations must work in read-only folders.
    const auto portable = GetExeDir() / L"portable.flag";
    std::error_code ec;
    if (fs::exists(portable, ec)) return GetExeDir() / CONFIG_FILE;
    PWSTR local = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local))) {
        const auto path = fs::path(local) / L"TrafficMonitorPlus" / CONFIG_FILE;
        CoTaskMemFree(local);
        return path;
    }
    return GetExeDir() / CONFIG_FILE;
}

bool App::CopyToClipboard(HWND owner, const std::wstring& text) {
    if (!OpenClipboard(owner)) return false;
    EmptyClipboard();

    size_t bytes = (text.length() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!hMem) {
        CloseClipboard();
        return false;
    }

    void* pMem = GlobalLock(hMem);
    if (!pMem) { GlobalFree(hMem); CloseClipboard(); return false; }
    memcpy(pMem, text.c_str(), bytes);
    GlobalUnlock(hMem);
    if (!SetClipboardData(CF_UNICODETEXT, hMem)) {
        GlobalFree(hMem);
        CloseClipboard();
        return false;
    }

    CloseClipboard();
    return true;
}

// ────────────────────────────────────────────────────────
// Initialization
// ────────────────────────────────────────────────────────
bool App::Init(HINSTANCE hInstance) {
    m_hInstance = hInstance;
    m_shutting_down = false;
    m_worker_state = std::make_shared<UsageWorkerState>();

    // Detect Windows version and theme
    m_winver = GetWindowsVersion();

    // Initialize Theme
    m_theme = std::make_unique<Theme>();
    m_theme->Init(m_winver.is_dark_theme);

    // Load or initialize Settings
    fs::path cfgPath = GetConfigPath();
    m_usage.Load(cfgPath.parent_path() / L"usage-history.json");
    if (!fs::exists(cfgPath)) {
        const auto legacy = GetExeDir() / CONFIG_FILE;
        m_settings = std::make_unique<Settings>(fs::exists(legacy)
            ? Settings::LoadFromFile(legacy) : Settings::GetDefaults());
        m_settings->SaveToFile(cfgPath);
    } else {
        m_settings = std::make_unique<Settings>(Settings::LoadFromFile(cfgPath));
    }

    // Apply explicit theme override if configured
    if (m_settings->theme == L"dark") {
        m_theme->SetDarkMode(true);
    } else if (m_settings->theme == L"light") {
        m_theme->SetDarkMode(false);
    }

    // Auto-start registration on fresh start if enabled
    const bool startup_ready = !m_settings->auto_start || AutoStart::Enable(AutoStart::GetCurrentExePath());

    // Create hidden message window for message loop and timer
    if (!CreateMessageWindow()) {
        return false;
    }

    // Initialize Telemetry Engine
    m_telemetry = std::make_unique<TelemetryEngine>();
    m_telemetry->SetPollInterval(m_settings->poll_interval_ms);
    m_telemetry->SetTemperatureRequests(m_settings->items[DisplayItem::CpuTemp].enabled,m_settings->items[DisplayItem::GpuTemp].enabled);
    m_telemetry->Init();
    if (!m_settings->auto_select_adapter && !m_settings->adapter_name.empty()) {
        m_telemetry->SetAdapterFilter(m_settings->adapter_name);
    }

    // Initialize System Tray Icon with embedded custom logo
    HICON hAppIcon = static_cast<HICON>(LoadImageW(
        m_hInstance, MAKEINTRESOURCEW(101), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR
    ));
    if (!hAppIcon) {
        hAppIcon = LoadIconW(m_hInstance, MAKEINTRESOURCEW(101));
    }
    if (!hAppIcon) {
        hAppIcon = LoadIconW(nullptr, IDI_APPLICATION);
    }

    m_tray = std::make_unique<TrayIcon>();
    m_tray->on_menu_dismissed=[this](){if(m_taskbar)m_taskbar->RestoreAfterPopup();};
    m_tray->Init(m_hMsgWnd, hAppIcon);
    m_tray->UpdateTooltip(L"TrafficMonitorPlus");
    if (!startup_ready) m_tray->ShowNotification(L"Startup needs administrator setup", L"Restart this app as administrator once to configure elevated sign-in startup for temperature sensors.");

    // Initialize Taskbar Display Mode
    m_taskbar = std::make_unique<TaskbarHost>();
    m_taskbar->SetHideOnFullscreen(m_settings->hide_on_fullscreen);
    if (m_taskbar->Init(m_hInstance)) {
        m_taskbar_renderer = std::make_unique<D2DRenderer>();
        m_taskbar_renderer->Init(m_taskbar->GetHwnd(),D2DRenderer::Surface::BufferedLayered);
        m_taskbar_renderer->SetFont(m_settings->font_family,
                                    static_cast<float>(m_settings->font_size),
                                    m_settings->font_bold);

        m_taskbar->on_paint = [this](HWND h, HDC) { RenderTaskbar(h); };
        m_taskbar->on_click = [this]() {
            if (!m_taskbar || !m_flyout) return;
            RECT rc{};
            GetWindowRect(m_taskbar->GetHwnd(), &rc);
            POINT anchor{ (rc.left + rc.right) / 2, rc.top };
            if (m_process_monitor) {
                m_process_monitor->Poll();
                if (m_telemetry) {
                    m_flyout->UpdateData(m_telemetry->GetLatestData(), m_process_monitor->GetProcesses(), m_telemetry->GetLocalIP());
                }
            }
            if (m_flyout->network_usage_provider) {
                m_flyout->UpdateNetworkReport(m_flyout->network_usage_provider(false));
            }
            m_flyout->Show(anchor, true);
        };
        m_taskbar->on_right_click = [this]() { ShowTrayMenu(); };
        m_taskbar->Show();
    }

    // Initialize Text Renderer
    D2DRenderer* primary_renderer = m_taskbar_renderer.get();
    m_text_renderer = std::make_unique<TextRenderer>(primary_renderer, m_theme.get());
    m_text_renderer->SetSpeedFormat(m_settings->compact, m_settings->speed_unit_bits, m_settings->speed_unit);
    m_text_renderer->SetShowUnitSigns(m_settings->show_unit_signs);
    m_text_renderer->SetStatValueGap(m_settings->stat_value_gap);

    // Initialize Process Monitor (Top CPU hogs)
    m_process_monitor = std::make_unique<ProcessMonitor>();
    m_live_network = std::make_unique<LiveNetworkMonitor>();
    m_live_network->LoadHistory(GetConfigPath().parent_path() / L"live-app-usage.json");
    m_live_network->Start();

    // Initialize Interactive Flyout Dashboard
    m_flyout = std::make_unique<FlyoutWindow>();
    m_flyout->on_dismissed=[this](){if(m_taskbar){m_taskbar->SetUsageTooltipEnabled(true);m_taskbar->RestoreAfterPopup();}};
    m_flyout->on_shown=[this](){if(m_taskbar)m_taskbar->SetUsageTooltipEnabled(false);};
    m_flyout->on_retry_live_network=[this](){if(m_live_network&&m_live_network->Snapshot().state!=LiveNetworkState::Live){m_live_network->Stop();m_live_network->Poll(true);m_live_network->Start();}};
    m_flyout->Init(m_hInstance);
    m_flyout->on_copy_ip = [this]() {
        auto ip=m_telemetry->GetLocalIP();
        if(ip.empty()||ip==L"0.0.0.0")ip=GetLocalIPAddress();
        return !ip.empty()&&ip!=L"0.0.0.0"&&CopyToClipboard(m_flyout->GetHwnd(),ip);
    };
    m_flyout->on_data_usage = [this]() { OnMenuCommand(IDM_DATA_USAGE); };
    m_flyout->on_open_task_manager = [this]() { OnMenuCommand(IDM_TASK_MANAGER); };
    m_flyout->on_open_settings = [this]() { OnMenuCommand(IDM_SETTINGS); };
    m_flyout->on_open_drive = [this](wchar_t letter) {
        if(letter<L'A'||letter>L'Z')return;
        const std::wstring root=std::wstring(1,letter)+L":\\";
        ShellExecuteW(m_hMsgWnd,L"open",root.c_str(),nullptr,nullptr,SW_SHOWNORMAL);
    };
    m_flyout->network_usage_provider = [this](bool force) {
        return GetOrRequestDetailedUsage(UsageTimeRange::Today, UsageNetworkScope::ActiveOnly, force);
    };
    m_flyout->on_kill_target = [this](const std::wstring&, DWORD, bool) {
        if (m_process_monitor) {
            m_process_monitor->Poll();
        }
        return true;
    };

    // Initialize Settings Hub Window
    m_settings_window = std::make_unique<SettingsWindow>();
    m_settings_window->Init(m_hInstance);
    m_settings_window->on_layout_preview=[this](const Settings& draft){PreviewLayout(&draft);};
    m_settings_window->on_preview_finished=[this](){PreviewLayout(nullptr);};
    m_settings_window->live_settings = [this]() { return *m_settings; };
    m_settings_window->data_snapshot = [this]() { return m_telemetry->GetLatestData(); };
    m_settings_window->usage_snapshot = [this]() {
        const auto date = UsageHistory::LocalDate();
        const auto date_w = Utf8ToWide(date);
        const auto snap = GetUsageSnapshot();
        const bool today_valid = PreferWindowsUsage(snap.has_today && snap.query_date == date_w,snap.today,m_usage.Day(date));
        const auto today = today_valid ? snap.today : m_usage.Day(date);
        const auto tracked_month=m_usage.Month(date.substr(0,7));
        const auto month = PreferWindowsUsage(snap.has_month&&snap.query_month==Utf8ToWide(date.substr(0,7)),snap.month,tracked_month) ? snap.month : tracked_month;
        return std::make_pair(today, month);
    };
    m_settings_window->on_export_usage = [this](const fs::path& path) { return m_usage.ExportCsv(path); };
    m_settings_window->on_open_history = [this]() {
        const auto directory = GetConfigPath().parent_path().wstring();
        ShellExecuteW(m_hMsgWnd, L"open", directory.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    };
    m_settings_window->on_sensor_status = [this]() { OnMenuCommand(IDM_SENSOR_STATUS); };
    m_settings_window->on_repair_startup = [this]() { OnMenuCommand(IDM_SENSOR_ADMIN); };
    m_settings_window->startup_status = []() { return AutoStart::IsEnabled(); };
    m_settings_window->history_status = [this]() { return m_usage.Status(); };
    m_settings_window->is_native_windows_usage = [this]() {const auto snap=GetUsageSnapshot();const auto date=UsageHistory::LocalDate();return PreferWindowsUsage(snap.has_today&&snap.query_date==Utf8ToWide(date),snap.today,m_usage.Day(date));};
    m_settings_window->detailed_usage_snapshot = [this](UsageTimeRange range, UsageNetworkScope scope, bool force) {
        return GetOrRequestDetailedUsage(range, scope, force);
    };
    m_settings_window->on_export_detailed_usage = [](const fs::path& path, const NetworkUsageReport& report) {
        return WindowsNetworkUsage::ExportDetailedCsv(path, report);
    };
    m_settings_window->on_settings_changed = [this](const Settings& s) {
        const bool autostart_changed = (s.auto_start != m_settings->auto_start);
        bool autostart_succeeded = false;
        if (autostart_changed) {
            const bool ok = s.auto_start ? AutoStart::Enable(AutoStart::GetCurrentExePath()) : AutoStart::Disable();
            if (!ok) {
                MessageBoxW(m_hMsgWnd, L"Could not update Windows startup. Restart this app as administrator, then apply again.", L"Startup settings", MB_OK | MB_ICONERROR);
                return false;
            }
            autostart_succeeded = true;
        }
        if (!s.SaveToFile(GetConfigPath())) {
            // Roll back autostart if settings file write failed
            if (autostart_changed && autostart_succeeded) {
                if (m_settings->auto_start) {
                    AutoStart::Enable(AutoStart::GetCurrentExePath());
                } else {
                    AutoStart::Disable();
                }
            }
            MessageBoxW(m_hMsgWnd, L"Failed to save settings to configuration file.", L"Settings Error", MB_OK | MB_ICONERROR);
            return false;
        }
        Settings applied = s;
        applied.display_mode = DisplayMode::Taskbar;
        if (!m_taskbar) OnMenuCommand(IDM_SHOW_TASKBAR);
        *m_settings = applied;
        m_settings->SaveToFile(GetConfigPath());
        if (m_taskbar) m_taskbar->SetHideOnFullscreen(s.hide_on_fullscreen);
        if (m_taskbar_renderer)
            m_taskbar_renderer->SetFont(s.font_family, static_cast<float>(s.font_size), s.font_bold);
        m_text_renderer->SetSpeedFormat(s.compact, s.speed_unit_bits, s.speed_unit);
        m_text_renderer->SetShowUnitSigns(s.show_unit_signs);
        m_text_renderer->SetStatValueGap(s.stat_value_gap);
        m_theme->SetDarkMode(s.theme == L"dark" || (s.theme == L"auto" && IsSystemDarkTheme()));
        m_telemetry->SetAdapterFilter(s.auto_select_adapter ? L"" : s.adapter_name);
        m_telemetry->SetPollInterval(s.poll_interval_ms);
        m_telemetry->SetTemperatureRequests(s.items.at(DisplayItem::CpuTemp).enabled,s.items.at(DisplayItem::GpuTemp).enabled);
        StartUpdateCheck();
        if (!s.check_updates) m_update_notice.clear();
        if (m_timer_id) KillTimer(m_hMsgWnd, m_timer_id);
        m_timer_id = SetTimer(m_hMsgWnd, TIMER_ID, s.poll_interval_ms, nullptr);
        if (m_taskbar) {
            m_taskbar->SetOffset(s.taskbar_offset_x, s.taskbar_offset_y);
            m_taskbar->UpdatePosition();
            if (m_taskbar->IsVisible()) InvalidateRect(m_taskbar->GetHwnd(), nullptr, FALSE);
        }
        return true;
    };

    // Register Global Hotkey (Win + Shift + M) to toggle flyout anywhere
    RegisterHotKey(m_hMsgWnd, HOTKEY_ID, MOD_WIN | MOD_SHIFT, 'M');

    RefreshWindowsUsageAsync();
    m_last_win_usage_fetch = std::chrono::steady_clock::now();

    StartUpdateCheck();

    // Match the UI refresh timer to the selected telemetry interval.
    m_timer_id = SetTimer(m_hMsgWnd, TIMER_ID, m_settings->poll_interval_ms, nullptr);

    // Initial immediate poll & render
    OnTimer();

    return true;
}

// ────────────────────────────────────────────────────────
// Message Window
// ────────────────────────────────────────────────────────
void App::StartUpdateCheck() {
    if (m_settings->check_updates && !m_updater) {
        m_updater = std::make_unique<Updater>();
        m_updater->CheckForUpdatesAsync([this](const UpdateInfo& info) {
            if (info.update_available && m_tray) {
                std::wstring note = L"TrafficMonitorPlus " + info.latest_version + L" is available!";
                auto message = new std::wstring(std::move(note));
                if (!PostMessageW(m_hMsgWnd, WM_APP + 20, 0, reinterpret_cast<LPARAM>(message))) delete message;
            }
        });
    }

}

bool App::CreateMessageWindow() {
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc   = App::MessageWndProc;
    wc.hInstance     = m_hInstance;
    wc.lpszClassName = MSG_WND_CLASS;

    RegisterClassExW(&wc);

    m_hMsgWnd = CreateWindowExW(
        0, MSG_WND_CLASS, L"TrafficMonitorPlus_Heartbeat",
        0, 0, 0, 0, 0,
        nullptr, nullptr, m_hInstance, this
    );

    if(m_hMsgWnd){
        if (m_worker_state) {
            m_worker_state->target_hwnd.store(m_hMsgWnd);
        }
        // A desktop shortcut running without elevation may request that this
        // elevated instance show Settings. No settings or exit command is exposed.
        ChangeWindowMessageFilterEx(m_hMsgWnd,RegisterWindowMessageW(SHOW_SETTINGS_MESSAGE),MSGFLT_ALLOW,nullptr);
    }
    return (m_hMsgWnd != nullptr);
}

LRESULT CALLBACK App::MessageWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    App& app = App::Instance();

    static UINT taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    static UINT showSettings = RegisterWindowMessageW(SHOW_SETTINGS_MESSAGE);
    if(msg==showSettings){app.OnMenuCommand(IDM_SETTINGS);return 0;}
    if (msg == taskbarCreated) {
        if (app.m_tray) app.m_tray->Restore();
        if (app.m_taskbar) {
            app.m_taskbar->UpdatePosition();
            app.m_taskbar->Show();
        }
        return 0;
    }
    switch (msg) {
        case WM_DEVICECHANGE:
            if(app.m_telemetry)app.m_telemetry->RequestAdapterRescan();
            return 0;
        case WM_APP + 20: {
            std::unique_ptr<std::wstring> notice(reinterpret_cast<std::wstring*>(lParam));
            if (!app.m_settings->check_updates) return 0;
            app.m_update_notice = *notice;
            app.UpdateTrayTooltip();
            return 0;
        }
        case WM_APP + 21: {
            if (!app.m_shutting_down.load() && app.m_settings_window) {
                app.m_settings_window->RefreshReadouts();
            }
            if (!app.m_shutting_down.load() && app.m_flyout && app.m_flyout->IsVisible()) {
                if (app.m_flyout->network_usage_provider) {
                    app.m_flyout->UpdateNetworkReport(app.m_flyout->network_usage_provider(false));
                }
            }
            return 0;
        }
        case WM_APP + 22: {
            std::unique_ptr<WindowsUsageSnapshot> snapshot(reinterpret_cast<WindowsUsageSnapshot*>(lParam));
            if (snapshot && !app.m_shutting_down.load()) {
                {
                    std::lock_guard<std::mutex> lock(app.m_usage_snapshot_mutex);
                    if (snapshot->has_usage) {
                        app.m_win_usage_snapshot = *snapshot;
                        app.m_win_usage_backoff_sec = 3;
                    } else {
                        app.m_win_usage_snapshot.is_stale = true;
                        app.m_win_usage_snapshot.error=snapshot->error;
                        app.m_win_usage_backoff_sec = std::min(app.m_win_usage_backoff_sec * 2, 30);
                    }
                }
                if (app.m_taskbar) {
                    const auto date = UsageHistory::LocalDate();
                    const auto date_w = Utf8ToWide(date);
                    app.m_taskbar->SetUsageTooltip(UsageTooltip(app.GetUsageSnapshot(),app.m_usage.Day(date),date_w));
                }
                if (app.m_settings_window) {
                    app.m_settings_window->RefreshReadouts();
                }
            }
            return 0;
        }
        case WM_TIMER:
            if (wParam == TIMER_ID) {
                app.OnTimer();
                return 0;
            }
            break;

        case WM_TMP_TRAYICON:
            if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_LBUTTONUP || LOWORD(lParam) == WM_CONTEXTMENU || LOWORD(lParam) == NIN_SELECT || LOWORD(lParam) == NIN_KEYSELECT) {
                app.ShowTrayMenu();
                return 0;
            }
            break;

        case WM_COMMAND:
            app.OnMenuCommand(LOWORD(wParam));
            return 0;

        case WM_HOTKEY:
            if (wParam == HOTKEY_ID && app.m_flyout) {
                RECT rc{};
                SystemParametersInfoW(SPI_GETWORKAREA, 0, &rc, 0);
                POINT pt{ (rc.left + rc.right) / 2, rc.bottom - 48 };
                app.m_flyout->Toggle(pt, true);
                return 0;
            }
            break;

        case WM_SETTINGCHANGE:
            if (lParam && wcscmp(reinterpret_cast<const wchar_t*>(lParam), L"ImmersiveColorSet") == 0) {
                if (app.m_settings->theme == L"auto") {
                    bool dark = IsSystemDarkTheme();
                    app.m_theme->SetDarkMode(dark);
                    if (app.m_taskbar && app.m_taskbar->IsVisible()) {
                        InvalidateRect(app.m_taskbar->GetHwnd(), nullptr, FALSE);
                    }
                }
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ────────────────────────────────────────────────────────
// Message Loop & Shutdown
// ────────────────────────────────────────────────────────
int App::Run() {
    MSG msg{};
    BOOL result;
    while ((result = GetMessageW(&msg, nullptr, 0, 0)) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return result == -1 ? 1 : static_cast<int>(msg.wParam);
}

void App::Shutdown() {
    m_shutting_down = true;
    if (m_worker_state) {
        m_worker_state->cancelled = true;
        m_worker_state->target_hwnd.store(nullptr);
    }
    const auto wait_start = std::chrono::steady_clock::now();
    while (m_worker_state && m_worker_state->active_workers.load() > 0 &&
           std::chrono::steady_clock::now() - wait_start < std::chrono::seconds(2)) {
        Sleep(10);
    }

    if (m_telemetry) {
        const auto data = m_telemetry->GetLatestData();
        m_usage.Observe(UsageHistory::LocalDate(), data.total_session_download, data.total_session_upload);
        m_usage.Save(GetConfigPath().parent_path() / L"usage-history.json");
    }
    // Stop the updater worker before destroying objects used by its callback.
    m_updater.reset();
    MSG pending{};
    while (PeekMessageW(&pending, m_hMsgWnd, WM_APP + 20, WM_APP + 20, PM_REMOVE))
        delete reinterpret_cast<std::wstring*>(pending.lParam);
    while (PeekMessageW(&pending, m_hMsgWnd, WM_APP + 22, WM_APP + 22, PM_REMOVE))
        delete reinterpret_cast<WindowsUsageSnapshot*>(pending.lParam);
    if (m_timer_id && m_hMsgWnd) {
        KillTimer(m_hMsgWnd, m_timer_id);
        m_timer_id = 0;
    }

    if (m_hMsgWnd) {
        UnregisterHotKey(m_hMsgWnd, HOTKEY_ID);
    }

    if (m_settings) {
        m_settings->SaveToFile(GetConfigPath());
    }

    m_settings_window.reset();
    m_flyout.reset();
    if(m_live_network){m_live_network->Stop();m_live_network->Poll(true);m_live_network->SaveHistory(GetConfigPath().parent_path()/L"live-app-usage.json");m_live_network.reset();}
    m_process_monitor.reset();
    m_taskbar.reset();
    m_tray.reset();
    m_text_renderer.reset();
    m_taskbar_renderer.reset();
    m_telemetry.reset();

    if (m_hMsgWnd) {
        DestroyWindow(m_hMsgWnd);
        m_hMsgWnd = nullptr;
    }
}

// ────────────────────────────────────────────────────────
// Polling & Tooltip Update
// ────────────────────────────────────────────────────────
WindowsUsageSnapshot App::GetUsageSnapshot() const {
    std::lock_guard<std::mutex> lock(m_usage_snapshot_mutex);
    return m_win_usage_snapshot;
}

void App::RefreshWindowsUsageAsync() {
    if (m_shutting_down.load() || !m_worker_state) return;
    if (m_worker_state->cancelled.load() || m_worker_state->fetching_summary.exchange(true)) return;

    auto state = m_worker_state;
    state->active_workers.fetch_add(1);
    try {
        std::thread([state]() {
            struct WorkerGuard {
                std::shared_ptr<UsageWorkerState> s;
                ~WorkerGuard() {
                    s->fetching_summary = false;
                    s->active_workers.fetch_sub(1);
                }
            } guard{ state };

            if (state->cancelled.load()) return;
            WindowsUsageSummary summary{};
            const bool success = WindowsNetworkUsage::FetchUsageSummary(summary);
            if (!state->cancelled.load()) {
                HWND target = state->target_hwnd.load();
                if (target && !state->cancelled.load()) {
                    auto* snapshot = new WindowsUsageSnapshot{
                        summary.today,
                        summary.this_month,
                        success && (summary.today_valid || summary.month_valid),
                        success && summary.today_valid,
                        success && summary.month_valid,
                        std::chrono::system_clock::now(),
                        summary.date,
                        summary.month,
                        !success,
                        summary.error_message
                    };
                    if (!PostMessageW(target, WM_APP + 22, 0, reinterpret_cast<LPARAM>(snapshot))) {
                        delete snapshot;
                    }
                }
            }
        }).detach();
    } catch (...) {
        state->fetching_summary = false;
        state->active_workers.fetch_sub(1);
    }
}

NetworkUsageReport App::GetOrRequestDetailedUsage(UsageTimeRange range, UsageNetworkScope scope, bool force_refresh) {
    if (!m_worker_state) return {};
    const auto key = std::make_pair(static_cast<int>(range), static_cast<int>(scope));
    NetworkUsageReport snapshot{};
    bool need_fetch = false;
    {
        std::lock_guard<std::mutex> lock(m_worker_state->detailed_mutex);
        const auto it = m_worker_state->detailed_cache.find(key);
        const auto now = std::chrono::steady_clock::now();
        if (it != m_worker_state->detailed_cache.end()) {
            snapshot = it->second.report;
            if (force_refresh || (now - it->second.fetched_at) >= std::chrono::seconds(15)) {
                need_fetch = true;
            }
        } else {
            need_fetch = true;
        }
    }
    if (need_fetch) {
        RequestDetailedUsageAsync(range, scope, force_refresh);
    }
    return snapshot;
}

void App::RequestDetailedUsageAsync(UsageTimeRange range, UsageNetworkScope scope, bool force_refresh) {
    if (m_shutting_down.load() || !m_worker_state) return;
    auto state = m_worker_state;
    if (state->cancelled.load()) return;

    const auto key = std::make_pair(static_cast<int>(range), static_cast<int>(scope));
    {
        std::lock_guard<std::mutex> lock(state->detailed_mutex);
        if (!force_refresh) {
            const auto it = state->detailed_cache.find(key);
            if (it != state->detailed_cache.end() &&
                (std::chrono::steady_clock::now() - it->second.fetched_at) < std::chrono::seconds(15)) {
                return;
            }
        }
        if (state->detailed_inflight.count(key) > 0) return;
        state->detailed_inflight.insert(key);
    }

    state->active_workers.fetch_add(1);
    try {
        std::thread([state, range, scope, key]() {
            struct Guard {
                std::shared_ptr<UsageWorkerState> s;
                std::pair<int, int> k;
                ~Guard() {
                    {
                        std::lock_guard<std::mutex> lock(s->detailed_mutex);
                        s->detailed_inflight.erase(k);
                    }
                    s->active_workers.fetch_sub(1);
                }
            } guard{ state, key };

            if (state->cancelled.load()) return;
            NetworkUsageReport report{};
            const bool success = WindowsNetworkUsage::FetchDetailedUsage(range, scope, report);
            if (!state->cancelled.load()) {
                {
                    std::lock_guard<std::mutex> lock(state->detailed_mutex);
                    if (success) {
                        state->detailed_cache[key] = DetailedUsageCacheEntry{ report, std::chrono::steady_clock::now() };
                    } else {
                        // Mark existing cache entry as Stale with backoff, or record Failed entry
                        auto it = state->detailed_cache.find(key);
                        if (it != state->detailed_cache.end()) {
                            it->second.report.status = it->second.report.ready ? ReportStatus::Stale : ReportStatus::Failed;
                            it->second.fetched_at = std::chrono::steady_clock::now();
                        } else {
                            NetworkUsageReport failed_rep{};
                            failed_rep.range = range;
                            failed_rep.scope = scope;
                            failed_rep.status = ReportStatus::Failed;
                            failed_rep.ready = false;
                            failed_rep.error_message = L"Unable to query Windows network usage provider.";
                            state->detailed_cache[key] = DetailedUsageCacheEntry{ failed_rep, std::chrono::steady_clock::now() };
                        }
                    }
                }
                HWND target = state->target_hwnd.load();
                if (target && !state->cancelled.load()) {
                    PostMessageW(target, WM_APP + 21, 0, 0);
                }
            }
        }).detach();
    } catch (...) {
        std::lock_guard<std::mutex> lock(state->detailed_mutex);
        state->detailed_inflight.erase(key);
        state->active_workers.fetch_sub(1);
    }
}

void App::OnTimer() {
    if (m_telemetry) {
        m_telemetry->Poll();
        const auto data = m_telemetry->GetLatestData();
        m_usage.Observe(UsageHistory::LocalDate(), data.total_session_download, data.total_session_upload);
        const auto now = std::chrono::steady_clock::now();
        if (now - m_last_win_usage_fetch >= std::chrono::seconds(m_win_usage_backoff_sec)) {
            m_last_win_usage_fetch = now;
            RefreshWindowsUsageAsync();
        }
        if (m_taskbar) {
            const auto date = UsageHistory::LocalDate();
            const auto date_w = Utf8ToWide(date);
            const auto snap = GetUsageSnapshot();
            m_taskbar->SetUsageTooltip(UsageTooltip(snap,m_usage.Day(date),date_w));
        }
        if (now - m_usage_saved >= std::chrono::seconds(30)) {
            m_usage.Save(GetConfigPath().parent_path() / L"usage-history.json");
            if(m_live_network)m_live_network->SaveHistory(GetConfigPath().parent_path()/L"live-app-usage.json");
            m_usage_saved = now;
        }
    }

    if(m_live_network){m_live_network->Poll();if(m_flyout)m_flyout->UpdateLiveNetwork(m_live_network->Snapshot());}
    if (m_process_monitor && m_flyout && m_flyout->IsVisible()) {
        m_process_monitor->Poll();
    }
    if (m_settings_window) m_settings_window->RefreshReadouts();

    if (m_settings && m_settings->notifications_enabled && m_tray && m_telemetry) {
        const auto data = m_telemetry->GetLatestData();
        auto now = std::chrono::steady_clock::now();
        if (data.cpu_usage_percent >= m_settings->cpu_alert_threshold) {
            if (m_cpu_alert_since.time_since_epoch().count() == 0) m_cpu_alert_since = now;
            if (!m_cpu_alert_sent && std::chrono::duration_cast<std::chrono::seconds>(now - m_cpu_alert_since).count() >= m_settings->cpu_alert_sustained_sec) {
                m_tray->ShowNotification(L"High CPU usage", std::format(L"CPU usage is {:.0f}%", data.cpu_usage_percent));
                m_cpu_alert_sent = true;
            }
        } else { m_cpu_alert_since = {}; m_cpu_alert_sent = false; }
        if (data.ram_usage_percent >= m_settings->ram_alert_threshold) {
            if (!m_ram_alert_sent) m_tray->ShowNotification(L"High memory usage", std::format(L"RAM usage is {:.0f}%", data.ram_usage_percent));
            m_ram_alert_sent = true;
        } else m_ram_alert_sent = false;
        if (data.cpu_temp_celsius >= m_settings->temp_alert_threshold) {
            if (!m_temp_alert_sent) m_tray->ShowNotification(L"High CPU temperature", std::format(L"CPU temperature is {:.0f}°C", data.cpu_temp_celsius));
            m_temp_alert_sent = true;
        } else m_temp_alert_sent = false;
    }

    if (!m_menu_open) {
        if (m_taskbar && m_settings) {
            // TaskbarHost owns fullscreen/auto-hide and shell stacking policy.
            // A second scan here used to clear requested visibility for shell
            // panels or fullscreen windows behind an exposed taskbar, preventing
            // the host's recovery timer from restoring the widget.
            bool show = m_settings->display_mode == DisplayMode::Taskbar;
            if (show && !m_taskbar->IsRequestedVisible()) m_taskbar->Show();
            if (!show && m_taskbar->IsRequestedVisible()) m_taskbar->Hide();
        }
    }

    if (m_flyout && m_telemetry) {
        m_flyout->UpdateData(
            m_telemetry->GetLatestData(),
            m_process_monitor ? m_process_monitor->GetProcesses() : std::vector<ProcessInfo>{},
            m_telemetry->GetLocalIP()
        );
    }

    {
        if (m_taskbar && m_taskbar->IsVisible()) {
            m_taskbar->SetOffset(m_layout_preview?m_layout_preview->x:m_settings->taskbar_offset_x,
                                 m_layout_preview?m_layout_preview->y:m_settings->taskbar_offset_y);
            InvalidateRect(m_taskbar->GetHwnd(), nullptr, FALSE);
        }
    }

    // Refresh tray tooltip every 5 ticks
    if (++m_tick_count >= 5) {
        m_tick_count = 0;
        UpdateTrayTooltip();
    }
}

void App::UpdateTrayTooltip() {
    if (!m_tray || !m_telemetry) return;

    const auto& data = m_telemetry->GetLatestData();
    std::wstring tip = std::format(
        L"TrafficMonitorPlus\n\u2193 {}  \u2191 {}\nCPU: {:.0f}%  RAM: {:.0f}%",
        FormatSpeed(data.download_bytes_per_sec),
        FormatSpeed(data.upload_bytes_per_sec),
        data.cpu_usage_percent,
        data.ram_usage_percent
    );
    if (!m_update_notice.empty()) tip = m_update_notice + L"\n" + tip;
    m_tray->UpdateTooltip(tip);
}

// ────────────────────────────────────────────────────────
// Rendering
// ────────────────────────────────────────────────────────
void App::RenderTaskbar(HWND hwnd) {
    if (!m_taskbar_renderer || !m_text_renderer || !m_telemetry || !m_settings) return;
    if(!m_taskbar_renderer->IsBoundTo(hwnd)&&!m_taskbar_renderer->Init(hwnd,D2DRenderer::Surface::BufferedLayered))return;

    RECT rc{};
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    if (width <= 0 || height <= 0) return;

    m_taskbar_renderer->Resize(static_cast<UINT>(width), static_cast<UINT>(height));
    float scale = DpiHelper::GetScaleFactor(hwnd);
    width = static_cast<int>(width / scale);
    height = static_cast<int>(height / scale);
    m_text_renderer->SetRenderer(m_taskbar_renderer.get());

    const Settings& view=m_visual_preview?*m_visual_preview:*m_settings;
    m_text_renderer->SetSpeedFormat(view.compact,view.speed_unit_bits,view.speed_unit);
    m_text_renderer->SetShowUnitSigns(view.show_unit_signs);
    m_text_renderer->SetStatValueGap(view.stat_value_gap);
    m_taskbar_renderer->SetFont(view.font_family,static_cast<float>(view.font_size),view.font_bold);
    const auto& data = m_telemetry->GetLatestData();
    auto enabled_items = view.GetEnabledItemsSorted();

    m_text_renderer->SetThresholds(view);
    auto render_items = m_text_renderer->PrepareItems(data, enabled_items, view.thermal_colors,
        &view.custom_label_colors, &view.custom_value_colors);

    LayoutDirection layout = view.layout;
    RECT taskbarRect{};
    HWND shellTaskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (shellTaskbar && GetWindowRect(shellTaskbar, &taskbarRect) &&
        taskbarRect.bottom-taskbarRect.top > taskbarRect.right-taskbarRect.left)
        layout = LayoutDirection::Vertical;
    // Buffered offscreen frame. The previous complete frame stays on screen
    // until EndDraw atomically publishes this one.
    if (!m_taskbar_renderer->BeginDraw()) {
        return;
    }

    float rendered_width = 0.0f;
    float spacing = static_cast<float>(m_layout_preview?m_layout_preview->spacing:view.item_spacing);

    if (layout == LayoutDirection::AutoCompact) {
        float row_height=m_taskbar_renderer->MeasureText(L"Ag").height+1;
        for(const auto& item:render_items)row_height=(std::max)(row_height,item.height+1);
        const float available=(std::max)(2*row_height,(taskbarRect.bottom-taskbarRect.top-8)/scale);
        const int required=static_cast<int>(std::ceil((2*row_height+2)*scale));
        m_taskbar->SetContentHeight(required>taskbarRect.bottom-taskbarRect.top-6?required:0);
        rendered_width=m_text_renderer->RenderAutoCompact(render_items,2,1,spacing,available,row_height);
    } else if (layout == LayoutDirection::Stacked) {
        float row_height=m_taskbar_renderer->MeasureText(L"Ag").height+1;
        for(const auto& item:render_items)row_height=(std::max)(row_height,item.height+1);
        const int required=static_cast<int>(std::ceil((row_height*2+2)*scale));
        const int available=taskbarRect.bottom-taskbarRect.top-6;
        m_taskbar->SetContentHeight(required>available?required:0);
        rendered_width=m_text_renderer->RenderStacked(render_items,2,(std::max)(1.0f,(height-row_height*2)/2),spacing,row_height);
    } else if (layout == LayoutDirection::TwoRows) {
        m_taskbar->SetContentHeight(0);
        float row_height = m_taskbar_renderer->MeasureText(L"Ag").height + 2.0f;
        for (const auto& item : render_items) row_height = (std::max)(row_height, item.height + 2);
        // Vertically center 2 lines cleanly within taskbar height
        float start_y = (std::max)(2.0f, (height - row_height * 2) / 2.0f);
        rendered_width = m_text_renderer->RenderTwoRows(
            render_items, 2.0f, start_y, spacing, row_height
        );
    } else if (layout == LayoutDirection::Vertical) {
        float rendered_height = m_text_renderer->RenderVertical(render_items, 2.0f,
            static_cast<float>(view.vertical_margin) + 2.0f, spacing);
        m_taskbar->SetContentHeight(static_cast<int>(std::ceil((rendered_height + 8) * scale)));
        for (const auto& item : render_items) rendered_width = (std::max)(rendered_width, item.width);
    } else {
        m_taskbar->SetContentHeight(0);
        float y_pos = static_cast<float>(view.vertical_margin) + 2.0f;
        float h_spacing = spacing;
        rendered_width = m_text_renderer->RenderHorizontal(
            render_items, 2.0f, y_pos, h_spacing
        );
    }

    m_taskbar_renderer->EndDraw();

    // Value slots already include glyph padding. Keep just two DIPs at each edge
    // so the last column sits close to the tray and the overlay takes less space.
    int needed_width = static_cast<int>(std::ceil(rendered_width)) + 4;
    if (!m_menu_open && (needed_width > width || (width - needed_width) > 8)) {
        m_taskbar->SetContentWidth(static_cast<int>(std::ceil(needed_width * scale)));
    }
}

// ────────────────────────────────────────────────────────
// Tray Menu & Commands
// ────────────────────────────────────────────────────────
void App::ShowTrayMenu() {
    if (!m_tray || !m_settings || m_menu_open) return;

    const HWND owner = m_taskbar && m_taskbar->IsVisible() ? m_taskbar->GetHwnd() : m_hMsgWnd;
    m_menu_open = true;
    m_tray->ShowContextMenu(owner,m_settings->items);
    m_menu_open = false;
}

void App::PreviewLayout(const Settings* draft) {
    if(draft)m_visual_preview=*draft;else m_visual_preview.reset();
    const Settings& appearance=draft?*draft:*m_settings;
    m_theme->SetDarkMode(appearance.theme==L"dark"||(appearance.theme==L"auto"&&IsSystemDarkTheme()));
    if(draft)m_layout_preview=LayoutPreview{draft->taskbar_offset_x,draft->taskbar_offset_y,draft->item_spacing};
    else m_layout_preview.reset();
    if(m_taskbar){
        m_taskbar->SetOffset(m_layout_preview?m_layout_preview->x:m_settings->taskbar_offset_x,
                             m_layout_preview?m_layout_preview->y:m_settings->taskbar_offset_y);
        m_taskbar->UpdatePosition();
        InvalidateRect(m_taskbar->GetHwnd(),nullptr,FALSE);
        UpdateWindow(m_taskbar->GetHwnd());
    }
}

void App::OnMenuCommand(UINT cmd_id) {
    switch (cmd_id) {
        case IDM_DATA_USAGE: {
            const auto date = UsageHistory::LocalDate();
            const auto date_w = Utf8ToWide(date);
            const auto snap = GetUsageSnapshot();
            const bool today_valid = PreferWindowsUsage(snap.has_today && snap.query_date == date_w,snap.today,m_usage.Day(date));
            const auto today = today_valid ? snap.today : m_usage.Day(date);
            const auto tracked_month=m_usage.Month(date.substr(0,7));
            const bool month_valid=PreferWindowsUsage(snap.has_month&&snap.query_month==Utf8ToWide(date.substr(0,7)),snap.month,tracked_month);
            const auto month = month_valid ? snap.month : tracked_month;
            m_usage.Save(GetConfigPath().parent_path() / L"usage-history.json");
            const std::wstring notice = std::wstring(L"Today: ")+(today_valid?L"Windows recorded":L"app-tracked adapter traffic")+
                L"\nThis month: "+(month_valid?L"Windows recorded":L"app-tracked adapter traffic")+
                L"\nApp-tracked totals cover time this app was running; history saves across restarts.";
            const auto message = std::format(
                L"Today ({})\nDownload: {}\nUpload: {}\nCombined: {}\n\nThis month ({})\nDownload: {}\nUpload: {}\nCombined: {}\n\n"
                L"{}\n{}",
                Utf8ToWide(date), FormatMemory(today.download), FormatMemory(today.upload), FormatMemory(today.Combined()),
                Utf8ToWide(date.substr(0, 7)), FormatMemory(month.download), FormatMemory(month.upload), FormatMemory(month.Combined()),
                notice, m_usage.Status());
            MessageBoxW(m_flyout&&m_flyout->IsVisible()?m_flyout->GetHwnd():m_hMsgWnd, message.c_str(), L"Daily and monthly data usage", MB_OK | MB_ICONINFORMATION);
            break;
        }
        case IDM_SHOW_TASKBAR: {
            bool has_taskbar = (m_settings->display_mode == DisplayMode::Taskbar);

            if (has_taskbar) {
                m_settings->display_mode = DisplayMode::None;
                if (m_taskbar) m_taskbar->Hide();
            } else {
                m_settings->display_mode = DisplayMode::Taskbar;
                if (!m_taskbar) {
                    m_taskbar = std::make_unique<TaskbarHost>();
                    m_taskbar->SetHideOnFullscreen(m_settings->hide_on_fullscreen);
                    m_taskbar->Init(m_hInstance);
                    m_taskbar_renderer = std::make_unique<D2DRenderer>();
                    m_taskbar_renderer->Init(m_taskbar->GetHwnd(),D2DRenderer::Surface::BufferedLayered);
                    m_taskbar_renderer->SetFont(m_settings->font_family,
                                                static_cast<float>(m_settings->font_size),
                                                m_settings->font_bold);
                    m_taskbar->on_paint = [this](HWND h, HDC) { RenderTaskbar(h); };
                    m_taskbar->on_click = [this]() {
                        if (!m_taskbar || !m_flyout) return;
                        RECT rc{};
                        GetWindowRect(m_taskbar->GetHwnd(), &rc);
                        POINT anchor{ (rc.left + rc.right) / 2, rc.top };
                        if (m_process_monitor) {
                            m_process_monitor->Poll();
                            if (m_telemetry) {
                                m_flyout->UpdateData(m_telemetry->GetLatestData(), m_process_monitor->GetProcesses(), m_telemetry->GetLocalIP());
                            }
                        }
                        if (m_flyout->network_usage_provider) {
                            m_flyout->UpdateNetworkReport(m_flyout->network_usage_provider(false));
                        }
                        m_flyout->Show(anchor, true);
                    };
                    m_taskbar->on_right_click = [this]() { ShowTrayMenu(); };
                }
                m_taskbar->Show();
                m_taskbar->UpdatePosition();
            }
            m_settings->SaveToFile(GetConfigPath());
            break;
        }

        case IDM_SHOW_FLOATING:
            break;

        case IDM_SETTINGS:
            if (m_settings_window) {
                m_settings_window->Show(*m_settings);
                RequestDetailedUsageAsync(UsageTimeRange::Today, UsageNetworkScope::ActiveOnly, false);
            }
            break;

        case IDM_COPY_IP: {
            std::wstring ip = m_telemetry ? m_telemetry->GetLocalIP() : L"";
            if (ip.empty() || ip == L"0.0.0.0") {
                ip = GetLocalIPAddress();
            }
            if (!ip.empty()) {
                CopyToClipboard(m_hMsgWnd, ip);
                MessageBoxW(m_hMsgWnd, (L"Local IP copied to clipboard:\n" + ip).c_str(),
                            L"IP Address", MB_OK | MB_ICONINFORMATION);
            }
            break;
        }

        case IDM_TASK_MANAGER:
            ShellExecuteW(nullptr, L"open", L"taskmgr.exe", nullptr, nullptr, SW_SHOW);
            break;

        case IDM_AUTOSTART: {
            const bool requested = !AutoStart::IsEnabled();
            if (!(requested ? AutoStart::Enable(AutoStart::GetCurrentExePath()) : AutoStart::Disable())) {
                MessageBoxW(m_hMsgWnd, L"Could not update Windows startup. Restart the app as administrator and try again.", L"Startup settings", MB_OK | MB_ICONERROR);
                break;
            }
            m_settings->auto_start = requested;
            if (m_settings->auto_start) {
                AutoStart::Enable(AutoStart::GetCurrentExePath());
            } else {
                AutoStart::Disable();
            }
            m_settings->SaveToFile(GetConfigPath());
            break;
        }

        case IDM_EXIT:
            Shutdown();
            PostQuitMessage(0);
            break;

        case IDM_SENSOR_STATUS:
            if (m_telemetry) MessageBoxW(m_hMsgWnd, m_telemetry->GetHardwareStatus().c_str(),
                L"Hardware sensor status", MB_OK | MB_ICONINFORMATION);
            break;

        case IDM_SENSOR_ADMIN: {
            const auto exe = AutoStart::GetCurrentExePath();
            SHELLEXECUTEINFOW launch{sizeof(launch)};
            launch.lpVerb = L"runas";
            launch.lpFile = exe.c_str();
            launch.lpParameters = L"--wait-for-instance";
            launch.nShow = SW_SHOWNORMAL;
            if (ShellExecuteExW(&launch)) PostMessageW(m_hMsgWnd, WM_COMMAND, IDM_EXIT, 0);
            break;
        }

        default:
            // Display item toggles
            if (cmd_id >= IDM_DISPLAY_ITEM_BASE && cmd_id < IDM_DISPLAY_ITEM_BASE + DISPLAY_ITEM_COUNT) {
                DisplayItem item = static_cast<DisplayItem>(cmd_id - IDM_DISPLAY_ITEM_BASE);
                auto& cfg = m_settings->items[item];
                if(cfg.enabled && m_settings->GetEnabledItemsSorted().size()==1)break;
                cfg.enabled = !cfg.enabled;
                m_telemetry->SetTemperatureRequests(m_settings->items[DisplayItem::CpuTemp].enabled,m_settings->items[DisplayItem::GpuTemp].enabled);
                m_settings->SaveToFile(GetConfigPath());
                if (m_taskbar && m_taskbar->IsVisible()) InvalidateRect(m_taskbar->GetHwnd(), nullptr, FALSE);
            }
            break;
    }
}

// ────────────────────────────────────────────────────────
// Accessors
// ────────────────────────────────────────────────────────
Settings& App::GetSettings() { return *m_settings; }
TelemetryEngine& App::GetTelemetry() { return *m_telemetry; }
Theme& App::GetTheme() { return *m_theme; }

} // namespace tmp
