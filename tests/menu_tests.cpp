#include "windows/tray_icon.h"
#include <iostream>
int main(){
    std::map<tmp::DisplayItem,tmp::DisplayItemConfig> items;
    for(UINT i=0;i<tmp::DISPLAY_ITEM_COUNT;++i)items[static_cast<tmp::DisplayItem>(i)]={i%2==0,static_cast<int>(i),L""};
    auto menu=tmp::TrayIcon::CreateContextMenu(items);
    if(!menu)return 1;
    bool valid=GetMenuItemCount(menu)==11 && GetMenuItemID(menu,0)==tmp::IDM_SETTINGS &&
        GetMenuItemID(menu,10)==tmp::IDM_EXIT &&
        GetMenuState(menu,tmp::IDM_DATA_USAGE,MF_BYCOMMAND)==static_cast<UINT>(-1) &&
        GetMenuState(menu,tmp::IDM_DISPLAY_ITEM_BASE+static_cast<UINT>(tmp::DisplayItem::BatteryWattage),MF_BYCOMMAND)==static_cast<UINT>(-1) &&
        GetMenuState(menu,tmp::IDM_SHOW_TASKBAR,MF_BYCOMMAND)==static_cast<UINT>(-1) &&
        GetMenuState(menu,tmp::IDM_AUTOSTART,MF_BYCOMMAND)==static_cast<UINT>(-1);
    auto display=menu;
    valid=valid&&!GetSubMenu(menu,1);
    UINT position=1;
    for(UINT i=0;valid&&i<tmp::DISPLAY_ITEM_COUNT;++i){
        if(i==static_cast<UINT>(tmp::DisplayItem::BatteryWattage))continue;
        auto state=GetMenuState(display,tmp::IDM_DISPLAY_ITEM_BASE+i,MF_BYCOMMAND);
        valid=GetMenuItemID(display,position++)==tmp::IDM_DISPLAY_ITEM_BASE+i && state!=static_cast<UINT>(-1) &&
            !!(state&MF_CHECKED)==(i%2==0) && !(state&MF_GRAYED);
    }
    DestroyMenu(menu);
    for(auto& [item,config]:items)config.enabled=item==tmp::DisplayItem::CpuUsage;
    menu=tmp::TrayIcon::CreateContextMenu(items);display=menu;
    valid=valid&&!!(GetMenuState(display,tmp::IDM_DISPLAY_ITEM_BASE+static_cast<UINT>(tmp::DisplayItem::CpuUsage),MF_BYCOMMAND)&MF_GRAYED);
    DestroyMenu(menu);
    if(!valid)std::cerr<<"Unexpected Display menu, check state or last-metric protection\n";

    // ─── Phase 3: Test production TrayIcon popup window keyboard accessibility ───
    std::vector<UINT> received_commands;
    WNDCLASSEXW spy_wc{sizeof(spy_wc)};
    spy_wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT {
        if (msg == WM_COMMAND) {
            auto* cmds = reinterpret_cast<std::vector<UINT>*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (cmds) cmds->push_back(LOWORD(wp));
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    };
    spy_wc.hInstance = GetModuleHandleW(nullptr);
    spy_wc.lpszClassName = L"MenuTestSpy";
    RegisterClassExW(&spy_wc);
    HWND spy_hwnd = CreateWindowExW(0, spy_wc.lpszClassName, L"Spy", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, spy_wc.hInstance, nullptr);
    SetWindowLongPtrW(spy_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&received_commands));

    tmp::TrayIcon tray;
    tray.Init(spy_hwnd, nullptr);
    tray.ShowContextMenu(spy_hwnd, items);
    HWND menu_hwnd = tray.GetMenuHwnd();
    if (!menu_hwnd || !IsWindowVisible(menu_hwnd)) {
        std::cerr << "Tray menu window not visible\n";
        return 1;
    }

    // Test Down arrow navigation
    SendMessageW(menu_hwnd, WM_KEYDOWN, VK_DOWN, 0); // selected_row becomes 1 (Download speed)
    if (tray.GetSelectedRow() != 1) { std::cerr << "VK_DOWN failed\n"; return 1; }

    // Test Up arrow navigation wraps to Exit.
    SendMessageW(menu_hwnd, WM_KEYDOWN, VK_UP, 0);
    if (tray.GetSelectedRow() != 10) { std::cerr << "VK_UP wrap failed\n"; return 1; }

    // Test VK_HOME
    SendMessageW(menu_hwnd, WM_KEYDOWN, VK_HOME, 0);
    if (tray.GetSelectedRow() != 1) { std::cerr << "VK_HOME failed\n"; return 1; }

    // Test Enter key activation on item 1 (DownloadSpeed)
    SendMessageW(menu_hwnd, WM_KEYDOWN, VK_RETURN, 0);
    if (received_commands.empty() || received_commands.back() != tmp::IDM_DISPLAY_ITEM_BASE + 0) {
        std::cerr << "VK_RETURN did not trigger IDM_DISPLAY_ITEM_BASE\n";
        return 1;
    }

    // CPU frequency retains its command after Battery power is removed.
    for (int k = 1; k < 8; ++k) SendMessageW(menu_hwnd, WM_KEYDOWN, VK_DOWN, 0);
    SendMessageW(menu_hwnd, WM_KEYDOWN, VK_RETURN, 0);
    if(received_commands.back()!=tmp::IDM_DISPLAY_ITEM_BASE+static_cast<UINT>(tmp::DisplayItem::CpuFrequency))return 1;
    // Settings now follows the eight visible metrics.
    SendMessageW(menu_hwnd, WM_KEYDOWN, VK_DOWN, 0);
    if (tray.GetSelectedRow() != 9) { std::cerr << "Navigation to Settings failed\n"; return 1; }
    SendMessageW(menu_hwnd, WM_KEYDOWN, VK_RETURN, 0);
    if (received_commands.back() != tmp::IDM_SETTINGS) { std::cerr << "Settings activation failed\n"; return 1; }
    if (IsWindowVisible(menu_hwnd)) { std::cerr << "Menu should close on Settings activation\n"; return 1; }

    // Reopen menu and test Last-metric protection with keyboard
    std::map<tmp::DisplayItem, tmp::DisplayItemConfig> single_item;
    for (UINT i = 0; i < tmp::DISPLAY_ITEM_COUNT; ++i) {
        single_item[static_cast<tmp::DisplayItem>(i)] = { i == 0, static_cast<int>(i), L"" };
    }
    tray.ShowContextMenu(spy_hwnd, single_item);
    received_commands.clear();
    SendMessageW(menu_hwnd, WM_KEYDOWN, VK_DOWN, 0); // row 1 (DownloadSpeed, the only enabled one)
    SendMessageW(menu_hwnd, WM_KEYDOWN, VK_RETURN, 0); // Must be protected!
    if (!received_commands.empty()) {
        std::cerr << "Last-metric was not protected against keyboard toggle\n";
        return 1;
    }

    // Test Exit with confirmed exit
    bool exit_confirmed = false;
    tray.confirm_exit = [&]() { exit_confirmed = true; return true; };
    SendMessageW(menu_hwnd, WM_KEYDOWN, VK_END, 0); // row 10 (Exit)
    if (tray.GetSelectedRow() != 10) { std::cerr << "VK_END failed\n"; return 1; }
    SendMessageW(menu_hwnd, WM_KEYDOWN, VK_RETURN, 0);
    if (!exit_confirmed || received_commands.empty() || received_commands.back() != tmp::IDM_EXIT) {
        std::cerr << "Exit confirmation/command failed\n";
        return 1;
    }

    // Test Escape dismissal
    bool dismissed = false;
    tray.on_menu_dismissed = [&]() { dismissed = true; };
    tray.ShowContextMenu(spy_hwnd, items);
    SendMessageW(menu_hwnd, WM_KEYDOWN, VK_ESCAPE, 0);
    if (!dismissed || IsWindowVisible(menu_hwnd)) {
        std::cerr << "VK_ESCAPE dismissal failed\n";
        return 1;
    }

    DestroyWindow(spy_hwnd);
    return valid?0:1;
}
