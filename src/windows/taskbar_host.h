// TrafficMonitorPlus - Taskbar Embedded Window Host
#pragma once
#include "pch.h"

namespace tmp {

class TaskbarHost {
public:
    TaskbarHost() = default;
    ~TaskbarHost();

    TaskbarHost(const TaskbarHost&) = delete;
    TaskbarHost& operator=(const TaskbarHost&) = delete;

    // Find the Windows taskbar and create our taskbar overlay window.
    bool Init(HINSTANCE hinstance);

    // Recalculates our position within the taskbar (call on resize / DPI change / tick).
    void UpdatePosition();
    void RestoreAfterPopup();

    // Sets the desired content width in pixels, triggers reposition.
    void SetContentWidth(int width);
    void SetContentHeight(int height);
    void SetOffset(int x, int y) {
        if (offset_x_ == x && offset_y_ == y) return;
        offset_x_ = x; offset_y_ = y; UpdatePosition();
    }
    void SetUsageTooltip(const std::wstring& text);
    void SetUsageTooltipEnabled(bool enabled);
    void SetHideOnFullscreen(bool hide) { hide_on_fullscreen_ = hide; }

    HWND GetHwnd() const { return hwnd_; }

    void Show();
    void Hide();
    bool IsVisible() const;
    bool IsRequestedVisible() const { return requested_visible_; }

    void Destroy();

    // Callbacks set by the main application.
    std::function<void(HWND, HDC)> on_paint;
    std::function<void()>          on_click;
    std::function<void()>          on_right_click;

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static void CALLBACK ShellEvent(HWINEVENTHOOK,DWORD,HWND,LONG,LONG,DWORD,DWORD);
    void WatchShell(DWORD process);
    bool SetVisibilityTimer(bool recovering);
    bool CanDisplayWithTaskbar() const;
    HWINEVENTHOOK foreground_hook_=nullptr,reorder_hook_=nullptr,location_hook_=nullptr,visibility_hook_=nullptr;
    ULONGLONG recovery_until_=0;
    UINT visibility_interval_=0;
    DWORD shell_process_=0;
    bool restore_pending_=false;
    bool requested_visible_=false;

    // Re-detect the taskbar windows and reposition.
    void RepositionInTaskbar();

    HWND      hwnd_           = nullptr;
    // A nearly transparent input surface catches clicks between the glyphs.
    HWND      input_hwnd_     = nullptr;
    HWND      hwnd_taskbar_   = nullptr;
    HWND      hwnd_notify_    = nullptr;
    HWND      tooltip_ = nullptr;
    HFONT     tooltip_font_ = nullptr;
    UINT      tooltip_dpi_ = 0;
    std::wstring tooltip_text_;
    bool tooltip_enabled_ = true;
    HINSTANCE hinstance_      = nullptr;
    ATOM      wnd_class_atom_ = 0;
    bool      hide_on_fullscreen_ = true;
    int       last_target_x_  = -999999;
    int       last_target_y_  = -999999;
    int       last_width_     = -1;
    int       last_height_    = -1;
    int       content_width_  = 210;
    int       content_height_ = 36;
    int       requested_height_ = 0;
    int       offset_x_ = 0, offset_y_ = 0;
    bool      class_registered_ = false;
    RECT      last_taskbar_rc_{};
    RECT      last_notify_rc_{};
};

} // namespace tmp
