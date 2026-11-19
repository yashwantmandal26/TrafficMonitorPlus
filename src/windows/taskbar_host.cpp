// TrafficMonitorPlus - Taskbar Embedded Window Host Implementation
#include "pch.h"
#include "windows/taskbar_host.h"
#include "windows/taskbar_visibility.h"
#include <uxtheme.h>

namespace tmp {

static constexpr const wchar_t* kTaskbarClassName = L"TrafficMonitorPlusTaskbar";
static constexpr int kTaskbarXPadding = 0;
static constexpr UINT kRestoreStacking=WM_APP+67;
static constexpr UINT_PTR kVisibilityTimer = 1;
static constexpr UINT kStableInterval = 2000;
static constexpr UINT kRecoveryInterval = 80;
static constexpr ULONGLONG kRecoveryDuration = 1200;
static std::map<HWINEVENTHOOK,TaskbarHost*> shell_watchers;

namespace {

bool IsWindowCloaked(HWND hwnd) {
    using DwmGetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, PVOID, DWORD);
    static const auto fn = []() -> DwmGetWindowAttributeFn {
        HMODULE dwm = GetModuleHandleW(L"dwmapi.dll");
        if (!dwm) dwm = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        return dwm ? reinterpret_cast<DwmGetWindowAttributeFn>(GetProcAddress(dwm, "DwmGetWindowAttribute")) : nullptr;
    }();
    if (!fn) return false;
    DWORD cloaked = 0;
    return SUCCEEDED(fn(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked != 0;
}

std::wstring GetWindowProcessNameLower(HWND hwnd) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid) return {};
    HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!proc) return {};
    wchar_t path[MAX_PATH]{};
    DWORD len = MAX_PATH;
    std::wstring name;
    if (QueryFullProcessImageNameW(proc, 0, path, &len) && len > 0) {
        std::wstring full(path, len);
        const size_t slash = full.find_last_of(L"\\/");
        name = (slash == std::wstring::npos) ? full : full.substr(slash + 1);
        for (wchar_t& ch : name) {
            if (ch >= L'A' && ch <= L'Z') ch = static_cast<wchar_t>(ch - L'A' + L'a');
        }
    }
    CloseHandle(proc);
    return name;
}

bool IsShellUiWindow(HWND hwnd) {
    if (!hwnd) return true;
    if (hwnd == GetShellWindow() || hwnd == GetDesktopWindow()) return true;

    wchar_t className[128]{};
    if (GetClassNameW(hwnd, className, 128) <= 0) return false;

    if(detail::IsShellSurface(className))return true;
    return detail::IsShellSurface(className,GetWindowProcessNameLower(hwnd));
}

bool IsBlockingFullscreenWindow(HWND hwnd, const RECT& monitor_rc,
                                HWND self_hwnd, HWND self_input, HWND self_tooltip,
                                bool ignore_zoomed_workarea_match) {
    if (!hwnd || !IsWindowVisible(hwnd) || IsIconic(hwnd)) return false;
    if (hwnd == self_hwnd || hwnd == self_input || hwnd == self_tooltip) return false;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0 || pid == GetCurrentProcessId()) return false;

    if (IsWindowCloaked(hwnd)) return false;

    const LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if ((ex & (WS_EX_LAYERED | WS_EX_TRANSPARENT)) == (WS_EX_LAYERED | WS_EX_TRANSPARENT)) {
        return false;
    }
    if (ex & WS_EX_LAYERED) {
        BYTE alpha = 255;
        DWORD flags = 0;
        if (GetLayeredWindowAttributes(hwnd, nullptr, &alpha, &flags) &&
            (flags & LWA_ALPHA) && alpha == 0) {
            return false;
        }
    }

    if (ignore_zoomed_workarea_match && IsZoomed(hwnd) &&
        (GetWindowLongPtrW(hwnd, GWL_STYLE) & WS_THICKFRAME)) {
        return false;
    }

    RECT wr{};
    if (!GetWindowRect(hwnd, &wr)) return false;
    if ((wr.right - wr.left) <= 0 || (wr.bottom - wr.top) <= 0) return false;

    RECT overlap{};
    if (!IntersectRect(&overlap, &wr, &monitor_rc)) return false;
    const bool covers_monitor = overlap.left <= monitor_rc.left + 2 &&
           overlap.top <= monitor_rc.top + 2 &&
           overlap.right >= monitor_rc.right - 2 &&
           overlap.bottom >= monitor_rc.bottom - 2;
    // Resolve shell process identity only for monitor-sized candidates. Avoid
    // opening every application process during the visibility watchdog tick.
    return covers_monitor&&!IsShellUiWindow(hwnd);
}

bool IsShellFlyoutActive(){
    const HWND foreground=GetForegroundWindow();
    wchar_t cls[128]{};GetClassNameW(foreground,cls,128);
    if(foreground&&IsShellUiWindow(foreground)&&wcscmp(cls,L"Shell_TrayWnd")!=0&&
       wcscmp(cls,L"Shell_SecondaryTrayWnd")!=0&&wcscmp(cls,L"Progman")!=0&&wcscmp(cls,L"WorkerW")!=0&&
       foreground!=GetDesktopWindow()&&foreground!=GetShellWindow())return true;
    for(const auto name:{L"ControlCenterWindow",L"NotifyIconOverflowWindow",L"TopLevelWindowForOverflowXamlIsland"}){
        HWND panel=FindWindowW(name,nullptr);
        if(panel&&IsWindowVisible(panel)&&!IsWindowCloaked(panel))return true;
    }
    return false;
}

bool IsTaskbarExposedAtPoints(HWND hwnd_taskbar, const RECT& tray) {
    if (!hwnd_taskbar) return false;
    const LONG width = tray.right - tray.left;
    const LONG height = tray.bottom - tray.top;
    if (width <= 0 || height <= 0) return false;

    const bool horizontal = width >= height;
    if (horizontal) {
        const LONG mid_y = tray.top + height / 2;
        for (int num : {1, 3, 4, 5, 7}) {
            const POINT pt{ tray.left + (width * num) / 8, mid_y };
            HWND hit = WindowFromPoint(pt);
            if (hit && (hit == hwnd_taskbar || GetAncestor(hit, GA_ROOT) == hwnd_taskbar)) {
                return true;
            }
        }
    } else {
        const LONG mid_x = tray.left + width / 2;
        for (int num : {1, 3, 4, 5, 7}) {
            const POINT pt{ mid_x, tray.top + (height * num) / 8 };
            HWND hit = WindowFromPoint(pt);
            if (hit && (hit == hwnd_taskbar || GetAncestor(hit, GA_ROOT) == hwnd_taskbar)) {
                return true;
            }
        }
    }
    return false;
}

bool IsWindowAbove(HWND upper, HWND lower) {
    if (!upper || !lower || upper == lower) return false;
    for (HWND w = GetWindow(lower, GW_HWNDPREV); w != nullptr; w = GetWindow(w, GW_HWNDPREV)) {
        if (w == upper) return true;
    }
    return false;
}

} // namespace

TaskbarHost::~TaskbarHost() {
    Destroy();
}

bool TaskbarHost::Init(HINSTANCE hinstance) {
    if (hwnd_) return true;

    hinstance_ = hinstance;

    // Register window class
    if (!class_registered_) {
        WNDCLASSEXW wc{};
        wc.cbSize        = sizeof(WNDCLASSEXW);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = TaskbarHost::WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = sizeof(void*);
        wc.hInstance     = hinstance;
        wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = kTaskbarClassName;

        wnd_class_atom_ = RegisterClassExW(&wc);
        if (!wnd_class_atom_) return false;
        class_registered_ = true;
    }

    // Try to find Shell_TrayWnd if Explorer is already running
    hwnd_taskbar_ = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (hwnd_taskbar_) {
        hwnd_notify_ = FindWindowExW(hwnd_taskbar_, nullptr, L"TrayNotifyWnd", nullptr);
    }

    // Taskbar geometry
    content_height_ = 36;
    if (hwnd_taskbar_) {
        RECT rcTaskbar{};
        if (GetWindowRect(hwnd_taskbar_, &rcTaskbar)) {
            int th = rcTaskbar.bottom - rcTaskbar.top;
            if (th > 0) content_height_ = th - 6;
        }
    }
    if (content_height_ < 24) content_height_ = 24;

    // Create top-level overlay window.
    // WS_POPUP with WS_EX_TOPMOST, WS_EX_TOOLWINDOW, WS_EX_NOACTIVATE, WS_EX_LAYERED.
    // Keep one stable popup surface. A cross-process WS_CHILD attachment to
    // Explorer can intermittently lose its redirection image on Windows 11.
    DWORD ex_style = WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED;
    DWORD style = WS_POPUP;

    hwnd_ = CreateWindowExW(
        ex_style,
        kTaskbarClassName,
        L"TrafficMonitorPlus Taskbar",
        style,
        0, 0,
        content_width_, content_height_,
        hwnd_taskbar_,
        nullptr,
        hinstance_,
        this
    );

    if (!hwnd_) return false;

    // Transparent pixels pass mouse input through to Explorer. Keep a solid,
    // alpha-1 input surface immediately above the visual window so the entire
    // widget rectangle (including spaces) behaves as one click target.
    input_hwnd_ = CreateWindowExW(ex_style, kTaskbarClassName,
        L"TrafficMonitorPlus Taskbar Input", style, 0, 0,
        content_width_, content_height_, hwnd_, nullptr, hinstance_, this);
    if (!input_hwnd_ || !SetLayeredWindowAttributes(input_hwnd_, 0, 1, LWA_ALPHA)) {
        Destroy();
        return false;
    }

    tooltip_ = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,hwnd_,nullptr,hinstance_,nullptr);
    if (tooltip_) {
        SetWindowTheme(tooltip_, L"", L"");
        SendMessageW(tooltip_, TTM_SETTIPBKCOLOR, RGB(12,18,29), 0);
        SendMessageW(tooltip_, TTM_SETTIPTEXTCOLOR, RGB(239,245,255), 0);
        SendMessageW(tooltip_, TTM_SETMAXTIPWIDTH, 0, 360);
        SendMessageW(tooltip_, TTM_SETDELAYTIME, TTDT_INITIAL, 500);
        SendMessageW(tooltip_, TTM_SETDELAYTIME, TTDT_AUTOPOP, 15000);
        tooltip_font_ = CreateFontW(-MulDiv(13,GetDpiForWindow(hwnd_),96),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
            DEFAULT_CHARSET,0,0,DEFAULT_QUALITY,0,L"Segoe UI");
        SendMessageW(tooltip_,WM_SETFONT,reinterpret_cast<WPARAM>(tooltip_font_),FALSE);
        tooltip_text_ = L"Today's usage is being measured...";
        TOOLINFOW tool{sizeof(tool)};
        tool.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        tool.hwnd = hwnd_; tool.uId = reinterpret_cast<UINT_PTR>(input_hwnd_);
        tool.lpszText = tooltip_text_.data();
        SendMessageW(tooltip_,TTM_ADDTOOLW,0,reinterpret_cast<LPARAM>(&tool));
        SendMessageW(tooltip_,TTM_ACTIVATE,tooltip_enabled_,0);
    }

    // The renderer configures the visual window's redirected transparency once
    // and copies completed offscreen frames into it. Never reset LAYERED here.

    RepositionInTaskbar();
    // Events handle immediate changes; a coalesced watchdog covers missed ones.
    if (!SetVisibilityTimer(false)) {
        Destroy();
        return false;
    }
    return true;
}

void TaskbarHost::Destroy() {
    if (hwnd_) KillTimer(hwnd_, kVisibilityTimer);
    for(auto hook:{foreground_hook_,reorder_hook_,location_hook_,visibility_hook_})if(hook){shell_watchers.erase(hook);UnhookWinEvent(hook);}
    foreground_hook_=reorder_hook_=location_hook_=visibility_hook_=nullptr;shell_process_=0;restore_pending_=false;requested_visible_=false;
    visibility_interval_=0;recovery_until_=0;
    last_target_x_=-999999;last_target_y_=-999999;last_width_=-1;last_height_=-1;
    if (tooltip_) { DestroyWindow(tooltip_); tooltip_ = nullptr; }
    if (tooltip_font_) { DeleteObject(tooltip_font_); tooltip_font_ = nullptr; }
    if (input_hwnd_) { DestroyWindow(input_hwnd_); input_hwnd_ = nullptr; }
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    if (class_registered_ && hinstance_) {
        UnregisterClassW(kTaskbarClassName, hinstance_);
        class_registered_ = false;
        wnd_class_atom_ = 0;
    }
}

void TaskbarHost::SetUsageTooltipEnabled(bool enabled) {
    tooltip_enabled_ = enabled;
    if(!tooltip_)return;
    if(!enabled)SendMessageW(tooltip_,TTM_POP,0,0);
    SendMessageW(tooltip_,TTM_ACTIVATE,enabled,0);
}

void TaskbarHost::SetUsageTooltip(const std::wstring& text) {
    if (!tooltip_ || tooltip_text_ == text) return;
    tooltip_text_ = text;
    TOOLINFOW tool{sizeof(tool)};
    tool.hwnd=hwnd_;tool.uId=reinterpret_cast<UINT_PTR>(input_hwnd_);
    tool.lpszText=tooltip_text_.data();
    SendMessageW(tooltip_,TTM_UPDATETIPTEXTW,0,reinterpret_cast<LPARAM>(&tool));
}

void TaskbarHost::Show() {
    requested_visible_=true;
    RestoreAfterPopup();
    if (hwnd_) {
        RepositionInTaskbar();
        if(!CanDisplayWithTaskbar()){ShowWindow(input_hwnd_,SW_HIDE);ShowWindow(hwnd_,SW_HIDE);return;}
        ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
        ShowWindow(input_hwnd_, SW_SHOWNOACTIVATE);
    }
}

void TaskbarHost::Hide() {
    requested_visible_=false;
    if (hwnd_) {
        ShowWindow(input_hwnd_, SW_HIDE);
        ShowWindow(hwnd_, SW_HIDE);
    }
}

bool TaskbarHost::IsVisible() const {
    return hwnd_ && IsWindowVisible(hwnd_);
}

void TaskbarHost::SetContentWidth(int width) {
    if (width < 1) width = 1;
    if (content_width_ == width) return;
    content_width_ = width;
    if (hwnd_) {
        RepositionInTaskbar();
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void TaskbarHost::SetContentHeight(int height) {
    height = (std::max)(0, height);
    if (requested_height_ == height) return;
    requested_height_ = height;
    if (hwnd_) {
        RepositionInTaskbar();
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void TaskbarHost::UpdatePosition() {
    // Explorer can destroy child surfaces when its taskbar is rebuilt.
    if(hwnd_&&!IsWindow(hwnd_)){
        const bool requested=requested_visible_;
        const auto tooltip=tooltip_text_;
        Destroy();
        if(Init(hinstance_)){
            SetUsageTooltip(tooltip);
            if(requested)Show();
        }
        return;
    }
    if (hwnd_) {
        RepositionInTaskbar();
    }
}

void TaskbarHost::RestoreAfterPopup(){
    if(!hwnd_||!requested_visible_)return;
    recovery_until_=GetTickCount64()+kRecoveryDuration;
    SetVisibilityTimer(true);
    if(restore_pending_)return;
    restore_pending_=true;
    if(!PostMessageW(hwnd_,kRestoreStacking,0,0))restore_pending_=false;
}
bool TaskbarHost::SetVisibilityTimer(bool recovering){
    const UINT interval=recovering?kRecoveryInterval:kStableInterval;
    if(visibility_interval_==interval)return true;
    if(!SetCoalescableTimer(hwnd_,kVisibilityTimer,interval,nullptr,recovering?10:250))return false;
    visibility_interval_=interval;
    return true;
}
void TaskbarHost::WatchShell(DWORD process){
    if(process==shell_process_&&foreground_hook_)return;
    for(auto hook:{foreground_hook_,reorder_hook_,location_hook_,visibility_hook_})if(hook){shell_watchers.erase(hook);UnhookWinEvent(hook);}
    shell_process_=process;
    foreground_hook_=SetWinEventHook(EVENT_SYSTEM_FOREGROUND,EVENT_SYSTEM_FOREGROUND,nullptr,ShellEvent,0,0,WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);
    // Global hooks also catch shell flyouts hosted outside Explorer, and a
    // foreground app entering/leaving fullscreen without a focus change.
    const DWORD flags=WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS;
    reorder_hook_=SetWinEventHook(EVENT_OBJECT_REORDER,EVENT_OBJECT_REORDER,nullptr,ShellEvent,0,0,flags);
    location_hook_=SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE,EVENT_OBJECT_LOCATIONCHANGE,nullptr,ShellEvent,0,0,flags);
    visibility_hook_=SetWinEventHook(EVENT_OBJECT_DESTROY,EVENT_OBJECT_HIDE,nullptr,ShellEvent,0,0,flags);
    for(auto hook:{foreground_hook_,reorder_hook_,location_hook_,visibility_hook_})if(hook)shell_watchers[hook]=this;
}
void CALLBACK TaskbarHost::ShellEvent(HWINEVENTHOOK hook,DWORD event,HWND window,LONG idObject,LONG,DWORD,DWORD){
    const auto found=shell_watchers.find(hook);if(found==shell_watchers.end()||!window)return;
    auto self=found->second;
    const bool taskbar=window==self->hwnd_taskbar_||GetAncestor(window,GA_ROOT)==self->hwnd_taskbar_;
    bool relevant=event==EVENT_SYSTEM_FOREGROUND;
    if(idObject==OBJID_WINDOW){
        if(event==EVENT_OBJECT_LOCATIONCHANGE)relevant=taskbar||window==GetForegroundWindow();
        else if(event==EVENT_OBJECT_REORDER)relevant=taskbar||window==GetDesktopWindow()||GetAncestor(window,GA_ROOT)==window;
        else if(event>=EVENT_OBJECT_DESTROY&&event<=EVENT_OBJECT_HIDE)
            relevant=taskbar||!IsWindow(window)||GetAncestor(window,GA_ROOT)==window;
    }
    // Coalesce bursts and defer work until the shell finishes the current event.
    if(relevant)self->RestoreAfterPopup();
}

bool TaskbarHost::CanDisplayWithTaskbar() const {
    RECT tray{};
    const HMONITOR screen = hwnd_taskbar_
        ? MonitorFromWindow(hwnd_taskbar_, MONITOR_DEFAULTTONEAREST)
        : MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitor{sizeof(monitor)};
    const bool has_monitor = GetMonitorInfoW(screen, &monitor) != FALSE;

    if (hwnd_taskbar_) {
        if (!IsWindow(hwnd_taskbar_) || !IsWindowVisible(hwnd_taskbar_) || IsWindowCloaked(hwnd_taskbar_)) {
            return false;
        }
        RECT visible{};
        if (!GetWindowRect(hwnd_taskbar_, &tray) || !has_monitor || !IntersectRect(&visible, &tray, &monitor.rcMonitor)) {
            return false;
        }
        // Explorer keeps WS_VISIBLE on while an auto-hide taskbar slides offscreen.
        const bool horizontal = (tray.right - tray.left) >= (tray.bottom - tray.top);
        const int visible_dim = horizontal ? (visible.bottom - visible.top) : (visible.right - visible.left);
        if (visible_dim < 10) return false;
    }

    if (!hide_on_fullscreen_) return true;
    if (!has_monitor) return true;

    if (hwnd_taskbar_) {
        // Walk strictly above hwnd_taskbar_ in Z-order so fullscreen video/games
        // stay hidden even if tray overflow, notifications, menus or a secondary
        // monitor temporarily take foreground focus.
        bool fullscreen_above_taskbar = false;
        for (HWND w = GetWindow(hwnd_taskbar_, GW_HWNDPREV); w != nullptr; w = GetWindow(w, GW_HWNDPREV)) {
            if (IsBlockingFullscreenWindow(w, monitor.rcMonitor, hwnd_, input_hwnd_, tooltip_, false)) {
                fullscreen_above_taskbar = true;
                break;
            }
        }
        if (fullscreen_above_taskbar && !IsTaskbarExposedAtPoints(hwnd_taskbar_, tray)) {
            return false;
        }
        const HWND foreground = GetForegroundWindow();
        if (IsBlockingFullscreenWindow(foreground, monitor.rcMonitor, hwnd_, input_hwnd_, tooltip_, false) &&
            !IsWindowAbove(hwnd_taskbar_, foreground) &&
            !IsTaskbarExposedAtPoints(hwnd_taskbar_, tray)) {
            return false;
        }
        return true;
    }

    for (HWND w = GetTopWindow(nullptr); w != nullptr; w = GetWindow(w, GW_HWNDNEXT)) {
        if (IsBlockingFullscreenWindow(w, monitor.rcMonitor, hwnd_, input_hwnd_, tooltip_, false)) {
            return false;
        }
    }
    return true;
}

void TaskbarHost::RepositionInTaskbar() {
    const HWND taskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (taskbar != hwnd_taskbar_) {
        hwnd_taskbar_ = taskbar;
        hwnd_notify_ = nullptr;
        if(hwnd_)SetWindowLongPtrW(hwnd_,GWLP_HWNDPARENT,reinterpret_cast<LONG_PTR>(taskbar));
    }

    DWORD shell_process = 0;
    if (hwnd_taskbar_) {
        GetWindowThreadProcessId(hwnd_taskbar_, &shell_process);
    }
    WatchShell(shell_process);

    // If taskbar is hidden (e.g. auto-hide) or covered by a fullscreen app, hide our window.
    if (!CanDisplayWithTaskbar()) {
        if (input_hwnd_ && IsWindowVisible(input_hwnd_)) {
            ShowWindow(input_hwnd_, SW_HIDE);
        }
        if (hwnd_ && IsWindowVisible(hwnd_)) {
            ShowWindow(hwnd_, SW_HIDE);
        }
        return;
    }

    RECT rcTaskbar{};
    if (hwnd_taskbar_) {
        if (!GetWindowRect(hwnd_taskbar_, &rcTaskbar)) return;
    } else {
        MONITORINFO mi{sizeof(mi)};
        HMONITOR hMon = MonitorFromPoint(POINT{0,0}, MONITOR_DEFAULTTOPRIMARY);
        if (GetMonitorInfoW(hMon, &mi)) {
            rcTaskbar = mi.rcMonitor;
            rcTaskbar.top = rcTaskbar.bottom - 48;
        } else {
            rcTaskbar = RECT{0, 1032, 1920, 1080};
        }
    }
    int taskbar_width = rcTaskbar.right - rcTaskbar.left;
    int taskbar_height = rcTaskbar.bottom - rcTaskbar.top;
    if (taskbar_width <= 0 || taskbar_height <= 0) return;

    content_height_ = requested_height_ > 0 ? requested_height_ : taskbar_height - 6;
    if (requested_height_ == 0 && content_height_ < 24) content_height_ = 24;

    // Query DPI for taskbar window
    UINT dpi = 96;
    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    if (hUser) {
        auto pGetDpi = reinterpret_cast<UINT(WINAPI*)(HWND)>(GetProcAddress(hUser, "GetDpiForWindow"));
        if (pGetDpi) {
            dpi = hwnd_taskbar_ ? pGetDpi(hwnd_taskbar_) : (hwnd_ ? pGetDpi(hwnd_) : 96);
        }
    }
    if (dpi == 0) dpi = 96;

    // Refresh TrayNotifyWnd
    if (tooltip_ && tooltip_dpi_ != dpi) {
        HFONT font = CreateFontW(-MulDiv(13,dpi,96),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
            DEFAULT_CHARSET,0,0,DEFAULT_QUALITY,0,L"Segoe UI");
        if (font) {
            SendMessageW(tooltip_,WM_SETFONT,reinterpret_cast<WPARAM>(font),FALSE);
            if (tooltip_font_) DeleteObject(tooltip_font_);
            tooltip_font_=font;tooltip_dpi_=dpi;
        }
        SendMessageW(tooltip_,TTM_SETMAXTIPWIDTH,0,MulDiv(360,dpi,96));
    }
    if (!hwnd_notify_ || !IsWindow(hwnd_notify_)) {
        hwnd_notify_ = FindWindowExW(hwnd_taskbar_, nullptr, L"TrayNotifyWnd", nullptr);
    }

    RECT rcNotify{};
    bool has_notify = false;
    if (hwnd_notify_ && IsWindowVisible(hwnd_notify_)) {
        if (GetWindowRect(hwnd_notify_, &rcNotify)) {
            int notify_w = rcNotify.right - rcNotify.left;
            if (notify_w > 10 && rcNotify.left > rcTaskbar.left) {
                has_notify = true;
            }
        }
    }

    int target_x = 0;
    int target_y = requested_height_ > 0
        ? rcTaskbar.bottom - content_height_
        : rcTaskbar.top + (taskbar_height - content_height_) / 2;

    if (has_notify) {
        // Place immediately to the left of the system tray notification area
        target_x = rcNotify.left - content_width_ - kTaskbarXPadding;
    } else {
        // Fallback for Windows 11 where TrayNotifyWnd may not report its rect:
        // Scaled estimate: ~240px tray width at 96 DPI
        int tray_width = MulDiv(240, static_cast<int>(dpi), 96);
        target_x = rcTaskbar.right - tray_width - content_width_ - kTaskbarXPadding;
    }

    // Boundary check: do not cross left margin
    int min_x = rcTaskbar.left + MulDiv(60, static_cast<int>(dpi), 96);
    if (target_x < min_x) {
        target_x = min_x;
    }

    int actual_width = content_width_;
    if (taskbar_height > taskbar_width) {
        actual_width = (std::min)(content_width_, taskbar_width);
        content_height_ = requested_height_ > 0 ? requested_height_ : 36;
        target_x = rcTaskbar.left + (taskbar_width - actual_width) / 2;
        target_y = has_notify ? rcNotify.top - content_height_ - 4 : rcTaskbar.bottom - 240 - content_height_;
        target_y = (std::max)(target_y, static_cast<int>(rcTaskbar.top));
    }
    // Position window with HWND_TOPMOST
    target_x += MulDiv(offset_x_, static_cast<int>(dpi), 96);
    target_y += MulDiv(offset_y_, static_cast<int>(dpi), 96);
    target_x = std::clamp(target_x, static_cast<int>(rcTaskbar.left), (std::max)(static_cast<int>(rcTaskbar.left), static_cast<int>(rcTaskbar.right) - actual_width));
    // User Y offsets may lift the widget above the taskbar. Keep it on its
    // monitor rather than clamping every requested offset into a six-pixel gap.
    MONITORINFO monitor{sizeof(monitor)};
    const HMONITOR hMon = hwnd_taskbar_ ? MonitorFromWindow(hwnd_taskbar_, MONITOR_DEFAULTTONEAREST) : MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
    const RECT bounds = GetMonitorInfoW(hMon, &monitor) ? monitor.rcMonitor : rcTaskbar;
    target_y = (std::max)(static_cast<int>(bounds.top), target_y);

    const bool geom_changed = (target_x != last_target_x_ || target_y != last_target_y_ ||
                               actual_width != last_width_ || content_height_ != last_height_);
    const bool need_show = requested_visible_ && (!IsWindowVisible(hwnd_) || !IsWindowVisible(input_hwnd_));

    bool stacking_buried = false;
    if (requested_visible_ && !need_show && !geom_changed) {
        // Shell dismiss surfaces can intercept hit testing while leaving the
        // widget visible. Do not fight their Z-order or raise Explorer itself.
        if(!IsShellFlyoutActive()){
            POINT test_pt{ target_x + 2, target_y + 2 };
            HWND hit = WindowFromPoint(test_pt);
            stacking_buried=hit&&hit!=input_hwnd_&&hit!=hwnd_&&hit!=tooltip_;
        }
    }

    if (geom_changed || need_show || stacking_buried) {
        POINT position{target_x,target_y};
        UINT flags = SWP_NOACTIVATE;
        if (!geom_changed && !need_show) flags |= SWP_NOMOVE | SWP_NOSIZE;
        if (!stacking_buried && !need_show) flags |= SWP_NOZORDER;
        if (requested_visible_ && need_show) flags |= SWP_SHOWWINDOW;

        // Move/reorder the visual and input surfaces as one window-position
        // transaction, avoiding an intermediate frame with mismatched geometry.
        HDWP batch=BeginDeferWindowPos(2);
        if(batch)batch=DeferWindowPos(batch,hwnd_,HWND_TOPMOST,position.x,position.y,actual_width,content_height_,flags);
        if(batch)batch=DeferWindowPos(batch,input_hwnd_,HWND_TOPMOST,position.x,position.y,actual_width,content_height_,flags);
        if(!batch||!EndDeferWindowPos(batch)){
            SetWindowPos(hwnd_,HWND_TOPMOST,position.x,position.y,actual_width,content_height_,flags);
            SetWindowPos(input_hwnd_,HWND_TOPMOST,position.x,position.y,actual_width,content_height_,flags);
        }

        last_target_x_ = target_x;
        last_target_y_ = target_y;
        last_width_ = actual_width;
        last_height_ = content_height_;
    }

    last_taskbar_rc_ = rcTaskbar;
    last_notify_rc_ = rcNotify;
}

LRESULT CALLBACK TaskbarHost::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    TaskbarHost* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<TaskbarHost*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<TaskbarHost*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (!self) {
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    switch (msg) {
        case WM_TIMER:
            if (hwnd == self->hwnd_ && wp == kVisibilityTimer) {
                if(self->requested_visible_)self->RepositionInTaskbar();
                self->SetVisibilityTimer(GetTickCount64()<self->recovery_until_);
            }
            return 0;
        case kRestoreStacking:
            self->restore_pending_=false;
            if(self->hwnd_&&self->requested_visible_){
                self->RepositionInTaskbar();
            }
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint(hwnd, &ps);
            if (hwnd == self->input_hwnd_) {
                FillRect(hdc, &ps.rcPaint, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            } else if (self->on_paint) {
                self->on_paint(hwnd, hdc);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_MOUSEACTIVATE:
            // Explorer must not activate/raise its taskbar before our callback.
            return MA_NOACTIVATE;

        case WM_LBUTTONUP: {
            if (self->tooltip_) SendMessageW(self->tooltip_, TTM_POP, 0, 0);
            if (self->on_click) {
                self->on_click();
            }
            return 0;
        }

        case WM_CONTEXTMENU:
        case WM_RBUTTONUP: {
            if (self->tooltip_) SendMessageW(self->tooltip_,TTM_POP,0,0);
            if (self->on_right_click) {
                self->on_right_click();
            }
            return 0;
        }

        case WM_DISPLAYCHANGE:
        case WM_SETTINGCHANGE: {
            self->RepositionInTaskbar();
            self->RestoreAfterPopup();
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_DESTROY:
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            return 0;

        default:
            break;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace tmp
