// TrafficMonitorPlus - System Tray Notification Icon
#pragma once
#include "pch.h"
#include "types.h"

namespace tmp {

class TrayIcon {
public:
    TrayIcon() = default;
    ~TrayIcon();

    TrayIcon(const TrayIcon&) = delete;
    TrayIcon& operator=(const TrayIcon&) = delete;

    // Creates the notification icon parented to the given window.
    bool Init(HWND hwnd_parent, HICON icon);

    // Updates tooltip text (truncated to 127 chars + null).
    void UpdateTooltip(const std::wstring& tip);
    void ShowNotification(const std::wstring& title, const std::wstring& text);

    // Shows the right-click context menu at cursor position.
    void ShowContextMenu(HWND hwnd, const std::map<DisplayItem,DisplayItemConfig>& items);
    static HMENU CreateContextMenu(const std::map<DisplayItem,DisplayItemConfig>& items = {});

    // Removes the notification icon from the tray.
    void Remove();
    void Restore();
    HWND GetMenuHwnd() const { return menu_window_; }
    int GetSelectedRow() const { return selected_row_; }
    std::function<void()> on_menu_dismissed;
    std::function<bool()> confirm_exit;

private:
    static LRESULT CALLBACK MenuProc(HWND,UINT,WPARAM,LPARAM);
    void ActivateRow(HWND hwnd, int row);
    HWND menu_window_=nullptr;
    const std::map<DisplayItem,DisplayItemConfig>* menu_items_=nullptr;
    int menu_row_height_=27;
    int selected_row_=-1;
    NOTIFYICONDATAW nid_{};
    bool            installed_ = false;
};

} // namespace tmp
