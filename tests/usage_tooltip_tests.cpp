#include "windows/taskbar_host.h"
#include <iostream>

int main() {
    INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES};InitCommonControlsEx(&controls);
    tmp::TaskbarHost host;
    if(!host.Init(GetModuleHandleW(nullptr)))return 1;
    struct Search {HWND owner;HWND tooltip;HWND input;};
    Search search{host.GetHwnd(),nullptr,nullptr};
    EnumThreadWindows(GetCurrentThreadId(),[](HWND hwnd,LPARAM value)->BOOL {
        auto& search=*reinterpret_cast<Search*>(value);
        wchar_t name[128]{};GetClassNameW(hwnd,name,128);
        if(GetWindow(hwnd,GW_OWNER)==search.owner&&wcscmp(name,TOOLTIPS_CLASSW)==0)search.tooltip=hwnd;
        if(GetWindow(hwnd,GW_OWNER)==search.owner&&wcscmp(name,L"TrafficMonitorPlusTaskbar")==0)search.input=hwnd;
        return TRUE;
    },reinterpret_cast<LPARAM>(&search));
    if(!search.input)search.input=FindWindowExW(FindWindowW(L"Shell_TrayWnd",nullptr),nullptr,L"TrafficMonitorPlusTaskbar",L"TrafficMonitorPlus Taskbar Input");
    if(!search.tooltip){std::cerr<<"Missing native usage tooltip\n";return 1;}
    if(!search.input){std::cerr<<"Missing full-area input surface\n";return 1;}
    const std::wstring message=L"Today (2026-11-05)\nDownload: 120 MB\nUpload: 30 MB\nCombined: 150 MB";
    host.SetUsageTooltip(message);
    wchar_t text[512]{};
    TOOLINFOW tool{sizeof(tool)};tool.hwnd=host.GetHwnd();tool.uId=reinterpret_cast<UINT_PTR>(search.input);tool.lpszText=text;
    SendMessageW(search.tooltip,TTM_GETTEXTW,512,reinterpret_cast<LPARAM>(&tool));
    if(message!=text || SendMessageW(search.tooltip,TTM_GETTOOLCOUNT,0,0)!=1){std::wcerr<<L"Initial text mismatch: ["<<text<<L"] tools="<<SendMessageW(search.tooltip,TTM_GETTOOLCOUNT,0,0)<<L"\n";return 1;}
    host.SetUsageTooltip(L"Updated daily total");
    SendMessageW(search.tooltip,TTM_GETTEXTW,512,reinterpret_cast<LPARAM>(&tool));
    if(wcscmp(text,L"Updated daily total")!=0){std::wcerr<<L"Update mismatch: ["<<text<<L"]\n";return 1;}
    host.Destroy();
    if(IsWindow(search.tooltip)||IsWindow(search.input)){std::cerr<<"Owned window not destroyed\n";return 1;}
    return 0;
}
