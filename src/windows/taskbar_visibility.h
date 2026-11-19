#pragma once
#include <string_view>
#include <initializer_list>

namespace tmp::detail {
// Shell panels can use a monitor-sized input/dismiss surface even though only
// the small flyout is painted. Their window bounds do not mean fullscreen video.
inline bool IsShellSurface(std::wstring_view window_class,std::wstring_view process_name_lower={}) {
    for(const auto name:{
        L"Shell_TrayWnd",L"Shell_SecondaryTrayWnd",L"NotifyIconOverflowWindow",
        L"TopLevelWindowForOverflowXamlIsland",L"#32768",L"Progman",L"WorkerW",
        L"MultitaskingViewFrame",L"XamlExplorerHostIslandWindow",L"ForegroundStaging",
        L"TaskSwitcherWnd",L"TaskListThumbnailWnd",L"EdgeUiInputTopWndClass",
        L"EdgeUiInputWndClass",L"Shell_LightDismissOverlay",L"LivePreview",
        L"ControlCenterWindow"
    })if(window_class==name)return true;

    // Shell hosting processes use both CoreWindow and composition-bridge hosts.
    // Do not restrict their exclusion to one UI framework's window class.
    for(const auto process:{L"shellexperiencehost.exe",L"shellhost.exe",
        L"startmenuexperiencehost.exe",L"searchhost.exe",L"searchapp.exe",L"textinputhost.exe"
    })if(process_name_lower==process)return true;

    // Explorer's file windows (including F11) remain ordinary application
    // windows. Only its shell CoreWindow surface is excluded here.
    return window_class==L"Windows.UI.Core.CoreWindow"&&process_name_lower==L"explorer.exe";
}
}
