#include "windows/taskbar_host.h"
#include "windows/taskbar_visibility.h"
#include "windows/flyout_window.h"
#include "rendering/dpi_helper.h"
#include <iostream>

int main(int argc, char** argv) {
    if(argc>=2&&std::string(argv[1])=="--observe-overlay"){
        // Read-only live regression probe: sample just the widget rectangle at
        // compositor cadence through multiple real telemetry updates.
        tmp::DpiHelper::Init();
        HWND visual=FindWindowW(L"TrafficMonitorPlusTaskbar",L"TrafficMonitorPlus Taskbar");
        if(!visual)visual=FindWindowExW(FindWindowW(L"Shell_TrayWnd",nullptr),nullptr,L"TrafficMonitorPlusTaskbar",L"TrafficMonitorPlus Taskbar");
        if(!visual||!IsWindowVisible(visual)){std::cerr<<"Overlay is not currently visible\n";return 1;}
        RECT bounds{};GetWindowRect(visual,&bounds);
        const int width=bounds.right-bounds.left,height=bounds.bottom-bounds.top;
        if(width<=0||height<=0)return 1;
        HDC screen=GetDC(nullptr),memory=CreateCompatibleDC(screen);
        std::cout<<"Capture device="<<GetDeviceCaps(screen,HORZRES)<<"x"<<GetDeviceCaps(screen,VERTRES)
            <<" widget="<<bounds.left<<","<<bounds.top<<" "<<width<<"x"<<height<<std::endl;
        BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;
        info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
        void* pixels=nullptr;HBITMAP bitmap=CreateDIBSection(memory,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
        if(!bitmap){DeleteDC(memory);ReleaseDC(nullptr,screen);return 1;}
        HGDIOBJ previous=SelectObject(memory,bitmap);
        int blank=0,hidden=0,moved=0,parent_changes=0,min_bright=width*height,max_bright=0;
        const HWND parent=GetParent(visual);
        constexpr int frames=600;
        for(int i=0;i<frames;++i){
            DwmFlush();
            RECT current{};GetWindowRect(visual,&current);
            if(!IsWindowVisible(visual))++hidden;
            if(!EqualRect(&bounds,&current))++moved;
            if(GetParent(visual)!=parent)++parent_changes;
            if(!BitBlt(memory,0,0,width,height,screen,bounds.left,bounds.top,SRCCOPY|CAPTUREBLT)){
                std::cerr<<"Frame capture failed: "<<GetLastError()<<"\n";return 1;
            }
            GdiFlush();
            int bright=0;
            const auto bytes=static_cast<const unsigned char*>(pixels);
            for(int p=0;p<width*height;++p)if((std::max)({bytes[p*4],bytes[p*4+1],bytes[p*4+2]})>=180)++bright;
            min_bright=(std::min)(min_bright,bright);max_bright=(std::max)(max_bright,bright);
            if(bright<10)++blank;
        }
        SelectObject(memory,previous);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(nullptr,screen);
        std::cout<<"Live overlay: "<<frames<<" compositor frames, blank="<<blank<<", hidden="<<hidden
            <<", geometry changes="<<moved<<", parent changes="<<parent_changes<<", bright pixels="<<min_bright<<".."<<max_bright<<"\n";
        return blank||hidden||moved||parent_changes?1:0;
    }
    if(argc>=2&&std::string(argv[1])=="--inspect-overlay"){
        tmp::DpiHelper::Init();
        HWND visual=FindWindowW(L"TrafficMonitorPlusTaskbar",L"TrafficMonitorPlus Taskbar");
        if(!visual)visual=FindWindowExW(FindWindowW(L"Shell_TrayWnd",nullptr),nullptr,L"TrafficMonitorPlusTaskbar",L"TrafficMonitorPlus Taskbar");
        RECT bounds{};GetWindowRect(visual,&bounds);
        std::wcout<<L"Widget "<<visual<<L" visible="<<IsWindowVisible(visual)<<L" rect="<<bounds.left<<L","<<bounds.top<<L","<<bounds.right<<L","<<bounds.bottom<<L"\n";
        const POINT points[]={{bounds.left+2,bounds.top+2},{(bounds.left+bounds.right)/2,(bounds.top+bounds.bottom)/2}};
        for(auto point:points){HWND hit=WindowFromPoint(point);wchar_t cls[256]{},rootcls[256]{};GetClassNameW(hit,cls,256);GetClassNameW(GetAncestor(hit,GA_ROOT),rootcls,256);std::wcout<<L"Hit "<<point.x<<L","<<point.y<<L" = "<<hit<<L" class="<<cls<<L" root="<<GetAncestor(hit,GA_ROOT)<<L" rootclass="<<rootcls<<L"\n";}
        std::wcout<<L"Foreground="<<GetForegroundWindow()<<L" Taskbar="<<FindWindowW(L"Shell_TrayWnd",nullptr)<<L"\n";
        int z=0;for(HWND w=GetTopWindow(nullptr);w;w=GetWindow(w,GW_HWNDNEXT),++z){
            RECT r{},intersection{};GetWindowRect(w,&r);wchar_t cls[256]{};GetClassNameW(w,cls,256);
            if(!IsWindowVisible(w)&&w!=visual&&wcscmp(cls,L"Shell_TrayWnd")!=0)continue;
            if(!IntersectRect(&intersection,&r,&bounds)&&wcscmp(cls,L"Shell_TrayWnd")!=0&&w!=GetForegroundWindow())continue;
            DWORD pid=0,cloaked=0;GetWindowThreadProcessId(w,&pid);DwmGetWindowAttribute(w,DWMWA_CLOAKED,&cloaked,sizeof(cloaked));
            std::wstring name;HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
            if(process){wchar_t path[32768]{};DWORD n=32768;if(QueryFullProcessImageNameW(process,0,path,&n))name=fs::path(path).filename().wstring();CloseHandle(process);}
            std::wcout<<z<<L" hwnd="<<w<<L" class="<<cls<<L" exe="<<name<<L" pid="<<pid<<L" visible="<<IsWindowVisible(w)<<L" cloaked="<<cloaked<<L" ex="<<std::hex<<GetWindowLongPtrW(w,GWL_EXSTYLE)<<std::dec<<L" owner="<<GetWindow(w,GW_OWNER)<<L" rect="<<r.left<<L","<<r.top<<L","<<r.right<<L","<<r.bottom<<L"\n";
        }
        return 0;
    }
    if (argc >= 4 && (std::string(argv[1]) == "--fullscreen-helper"||std::string(argv[1]) == "--shell-helper")) {
        const bool shell_panel=std::string(argv[1])=="--shell-helper";
        tmp::DpiHelper::Init();
        HANDLE ready_event = reinterpret_cast<HANDLE>(std::stoull(argv[2]));
        HANDLE stop_event = reinterpret_cast<HANDLE>(std::stoull(argv[3]));
        HMONITOR mon = MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi{sizeof(mi)};
        RECT bounds{0, 0, 1920, 1080};
        if (GetMonitorInfoW(mon, &mi)) bounds = mi.rcMonitor;

        if(shell_panel){
            WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"ControlCenterWindow";
            RegisterClassW(&wc);
        }
        HWND fs = CreateWindowExW(
            WS_EX_TOPMOST, shell_panel?L"ControlCenterWindow":L"STATIC", shell_panel?L"SimulatedQuickSettingsDismissSurface":L"SimulatedFullscreenVideo",
            WS_POPUP | WS_VISIBLE,
            bounds.left, bounds.top,
            bounds.right - bounds.left, bounds.bottom - bounds.top,
            nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        if (fs) {
            SetForegroundWindow(fs);
            UpdateWindow(fs);
        }
        if (ready_event) SetEvent(ready_event);
        for (;;) {
            const DWORD wait = MsgWaitForMultipleObjects(1, &stop_event, FALSE, 5000, QS_ALLINPUT);
            if (wait == WAIT_OBJECT_0 || wait == WAIT_FAILED) break;
            MSG msg{};
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
        if (fs) DestroyWindow(fs);
        return 0;
    }

    tmp::DpiHelper::Init();
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_WIN95_CLASSES};
    InitCommonControlsEx(&controls);
    int failures = 0;
    // Shell hosting is independent of the particular XAML/CoreWindow class.
    using tmp::detail::IsShellSurface;
    for(const auto process:{L"shellhost.exe",L"shellexperiencehost.exe",L"startmenuexperiencehost.exe",L"searchhost.exe",L"textinputhost.exe"}){
        if(!IsShellSurface(L"Windows.UI.Composition.DesktopWindowContentBridge",process)){
            std::cerr<<"Shell composition surface classified as an app\n";++failures;
        }
    }
    if(!IsShellSurface(L"ControlCenterWindow")||!IsShellSurface(L"Shell_LightDismissOverlay")||
       !IsShellSurface(L"Windows.UI.Core.CoreWindow",L"explorer.exe")||
       IsShellSurface(L"Chrome_WidgetWin_1",L"msedge.exe")||
       IsShellSurface(L"CabinetWClass",L"explorer.exe")||
       IsShellSurface(L"Windows.UI.Core.CoreWindow",L"video-player.exe")){
        std::cerr<<"Shell/application visibility classification regressed\n";++failures;
    }
    {
        tmp::TaskbarHost host;
        tmp::FlyoutWindow dashboard;
        if (!host.Init(GetModuleHandleW(nullptr)) || !dashboard.Init(GetModuleHandleW(nullptr))) return 1;
        // Exercise the production layered renderer, with transparent whitespace
        // around a small visible marker. Whitespace must still be clickable.
        tmp::D2DRenderer visual_renderer;
        host.on_paint = [&](HWND hwnd, HDC) {
            if(!visual_renderer.IsBoundTo(hwnd)&&!visual_renderer.Init(hwnd,tmp::D2DRenderer::Surface::BufferedLayered)){
                std::cerr<<"Could not initialize taskbar frame renderer\n";++failures;return;
            }
            RECT rect{}; GetClientRect(hwnd, &rect);
            visual_renderer.Resize(rect.right,rect.bottom);
            if(!visual_renderer.BeginDraw()){++failures;return;}
            visual_renderer.FillRoundedRect(D2D1::RectF(4,4,12,12),0,D2D1::ColorF(D2D1::ColorF::White));
            if(FAILED(visual_renderer.EndDraw())){std::cerr<<"Taskbar frame presentation failed\n";++failures;}
        };
        int clicks = 0, right_clicks = 0;
        host.on_click = [&]() {
            ++clicks;
            RECT rect{}; GetWindowRect(host.GetHwnd(), &rect);
            dashboard.Show({(rect.left+rect.right)/2, rect.top}, true);
        };
        host.on_right_click = [&](){++right_clicks;};
        host.SetContentWidth(180);
        host.Show();
        UpdateWindow(host.GetHwnd());
        struct Search { HWND owner; HWND input; } search{host.GetHwnd(),nullptr};
        EnumThreadWindows(GetCurrentThreadId(), [](HWND hwnd, LPARAM context)->BOOL {
            auto& search=*reinterpret_cast<Search*>(context);
            wchar_t name[128]{}; GetClassNameW(hwnd,name,128);
            if(GetWindow(hwnd,GW_OWNER)==search.owner && wcscmp(name,L"TrafficMonitorPlusTaskbar")==0)
                search.input=hwnd;
            return TRUE;
        }, reinterpret_cast<LPARAM>(&search));
        HWND input = search.input;
        if(!input)input=FindWindowExW(FindWindowW(L"Shell_TrayWnd",nullptr),nullptr,L"TrafficMonitorPlusTaskbar",L"TrafficMonitorPlus Taskbar Input");
        if (!input) {
            std::cerr << "Input surface is not above the transparent visual window\n";
            return 1;
        }
        UpdateWindow(input);
        DwmFlush();
        RECT rect{}, input_rect{};
        GetWindowRect(host.GetHwnd(), &rect); GetWindowRect(input, &input_rect);
        if (!EqualRect(&rect, &input_rect)) ++failures;
        for (POINT point : {POINT{rect.left+1, rect.top+1},
                            POINT{(rect.left+rect.right)/2, (rect.top+rect.bottom)/2},
                            POINT{rect.right-2, rect.bottom-2}}) {
            if (WindowFromPoint(point) != input) {
                wchar_t cls[128]{};GetClassNameW(WindowFromPoint(point),cls,128);
                std::wcerr << L"Whitespace hit "<<cls<<L" at "<<point.x<<L","<<point.y<<L" expected "<<input<<L" parent "<<GetParent(input)<<L" rect "<<input_rect.left<<L","<<input_rect.top<<L","<<input_rect.right<<L","<<input_rect.bottom<<L"\n";
                ++failures;
            }
        }
        if (SendMessageW(input, WM_MOUSEACTIVATE, 0, MAKELPARAM(HTCLIENT,WM_LBUTTONDOWN)) != MA_NOACTIVATE ||
            SendMessageW(host.GetHwnd(), WM_MOUSEACTIVATE, 0, MAKELPARAM(HTCLIENT,WM_RBUTTONDOWN)) != MA_NOACTIVATE) ++failures;
        for (UINT message : {WM_LBUTTONUP, WM_LBUTTONUP, WM_RBUTTONUP, WM_CONTEXTMENU}) {
            SendMessageW(input, message, 0, 0);
            MSG pending{};
            while (PeekMessageW(&pending,nullptr,0,0,PM_REMOVE)) {
                TranslateMessage(&pending); DispatchMessageW(&pending);
            }
            if (!dashboard.IsVisible() || !host.IsVisible() || !IsWindowVisible(input)) {
                std::cerr << "Click dismissed the dashboard or widget\n";
                ++failures;
            }
        }
        if (clicks != 2 || right_clicks != 2) ++failures;
        SendMessageW(dashboard.GetHwnd(),WM_ACTIVATE,WA_INACTIVE,0);
        if(dashboard.IsVisible()){std::cerr<<"Dashboard did not close on outside focus\n";++failures;}
        RECT anchor{};GetWindowRect(host.GetHwnd(),&anchor);dashboard.Show({anchor.left,anchor.top},true);
        RECT client{};GetClientRect(dashboard.GetHwnd(),&client);
        UpdateWindow(dashboard.GetHwnd());
        const float scale=(std::min)(client.right/float(dashboard.GetWidth()),client.bottom/float(dashboard.GetHeight()));
        SendMessageW(dashboard.GetHwnd(),WM_LBUTTONUP,0,MAKELPARAM(static_cast<int>(443*scale),static_cast<int>(31*scale)));
        if(dashboard.IsVisible()){std::cerr<<"Dashboard close control did not close\n";++failures;}
        host.SetContentWidth(160); host.SetOffset(-8,0); host.UpdatePosition();
        GetWindowRect(host.GetHwnd(), &rect); GetWindowRect(input, &input_rect);
        if (!EqualRect(&rect, &input_rect) || rect.right-rect.left != 160) ++failures;
        const int initial_y=rect.top;
        host.SetOffset(-8,-30);host.UpdatePosition();
        GetWindowRect(host.GetHwnd(),&rect);GetWindowRect(input,&input_rect);
        if(!EqualRect(&rect,&input_rect)||rect.top>=initial_y-10){std::cerr<<"Up offset was clamped to the taskbar\n";++failures;}
        host.Hide();
        if (IsWindowVisible(input) || host.IsVisible()) ++failures;
        host.Show();
        if (!IsWindowVisible(input) || !host.IsVisible()) ++failures;
        auto pump = [](ULONGLONG duration=800) {
            const auto until = GetTickCount64() + duration;
            do {
                MSG message{};
                while (PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
                    TranslateMessage(&message); DispatchMessageW(&message);
                }
                MsgWaitForMultipleObjects(0,nullptr,FALSE,20,QS_ALLINPUT);
            } while (GetTickCount64() < until);
        };
        // No Show(), telemetry tick or accessibility callback: the host must
        // repair externally hidden visual/input surfaces by itself.
        // Drain the earlier dashboard activation/close events before checking
        // whether visibility recovery changes foreground focus.
        pump();
        const HWND foreground = GetForegroundWindow();
        ShowWindow(host.GetHwnd(),SW_HIDE);
        pump(2400); // Missed events recover through the slower stable watchdog.
        if (!host.IsVisible() || !IsWindowVisible(input)) {
            std::cerr << "Visibility watchdog did not restore the hidden overlay\n"; ++failures;
        }
        ShowWindow(input,SW_HIDE);
        pump(2400);
        if (!IsWindowVisible(input)) { std::cerr << "Input surface did not recover\n"; ++failures; }
        // A new topmost surface must not permanently bury the widget.
        GetWindowRect(host.GetHwnd(), &rect);
        HWND cover_parent=(GetWindowLongPtrW(host.GetHwnd(),GWL_STYLE)&WS_CHILD)?GetParent(host.GetHwnd()):nullptr;
        POINT cover_position{rect.left,rect.top};
        if(cover_parent)ScreenToClient(cover_parent,&cover_position);
        HWND cover = CreateWindowExW(WS_EX_TOPMOST|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW,
            L"STATIC",L"Stacking regression",(cover_parent?WS_CHILD:WS_POPUP)|WS_VISIBLE,
            cover_position.x,cover_position.y,rect.right-rect.left,rect.bottom-rect.top,
            cover_parent,nullptr,GetModuleHandleW(nullptr),nullptr);
        pump(2400);
        DwmFlush();
        if (WindowFromPoint({rect.left+2,rect.top+2}) != input) {
            std::cerr << "Visibility watchdog did not repair topmost stacking\n"; ++failures;
        }
        DestroyWindow(cover);
        if (GetForegroundWindow() != foreground) { std::cerr << "Recovery stole focus\n"; ++failures; }

        // External fullscreen app (e.g. YouTube / VLC / game) must hide the widget,
        // even when a small popup (e.g. tray overflow) later takes foreground focus.
        auto run_fullscreen_case = [&](auto&& body,const wchar_t* mode=L"--fullscreen-helper") {
            SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
            HANDLE ready_event = CreateEventW(&sa, TRUE, FALSE, nullptr);
            HANDLE stop_event = CreateEventW(&sa, TRUE, FALSE, nullptr);
            wchar_t exe_path[MAX_PATH]{};
            GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
            std::wstring cmd = L"\"" + std::wstring(exe_path) + L"\" "+mode+L" " +
                std::to_wstring(reinterpret_cast<uintptr_t>(ready_event)) + L" " +
                std::to_wstring(reinterpret_cast<uintptr_t>(stop_event));
            STARTUPINFOW si{sizeof(si)};
            PROCESS_INFORMATION pi{};
            if (!ready_event || !stop_event ||
                !CreateProcessW(exe_path, cmd.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
                if (ready_event) CloseHandle(ready_event);
                if (stop_event) CloseHandle(stop_event);
                std::cerr << "Failed to launch fullscreen helper process\n";
                ++failures;
                return;
            }
            WaitForSingleObject(ready_event, 5000);
            body();
            SetEvent(stop_event);
            if (WaitForSingleObject(pi.hProcess, 5000) == WAIT_TIMEOUT) {
                TerminateProcess(pi.hProcess, 1);
            }
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            CloseHandle(ready_event);
            CloseHandle(stop_event);
        };

        host.SetHideOnFullscreen(true);
        run_fullscreen_case([&]() {
            pump();
            if (host.IsVisible() || IsWindowVisible(input)) {
                std::cerr << "Widget stayed visible over external fullscreen window\n"; ++failures;
            }
            // Small focused popup over fullscreen video must not unhide the widget.
            HWND popup = CreateWindowExW(WS_EX_TOPMOST, L"STATIC", L"PopupOverFullscreen",
                WS_POPUP | WS_VISIBLE, rect.left, (std::max)(0L, rect.top - 140), 220, 120,
                nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
            if (popup) SetForegroundWindow(popup);
            pump();
            if (host.IsVisible() || IsWindowVisible(input)) {
                std::cerr << "Widget reappeared when a small popup took focus over fullscreen\n"; ++failures;
            }
            if (popup) DestroyWindow(popup);
        });
        pump();
        if (!host.IsVisible() || !IsWindowVisible(input)) {
            std::cerr << "Widget did not reappear after fullscreen exited\n"; ++failures;
        }

        host.SetHideOnFullscreen(false);
        run_fullscreen_case([&]() {
            pump();
            if (!host.IsVisible() || !IsWindowVisible(input)) {
                std::cerr << "Widget hid even when hide_on_fullscreen was disabled\n"; ++failures;
            }
        });
        host.SetHideOnFullscreen(true);

        // Quick Settings may own a monitor-sized dismiss/input surface. Even
        // while that surface has focus and covers the tray, retain the widget
        // and recover its input surface without relying on telemetry callbacks.
        host.SetOffset(0,0);host.UpdatePosition();
        const HWND stable_parent=GetParent(host.GetHwnd());
        if((GetWindowLongPtrW(host.GetHwnd(),GWL_STYLE)&WS_CHILD)||
           GetWindow(host.GetHwnd(),GW_OWNER)!=FindWindowW(L"Shell_TrayWnd",nullptr)){
            std::cerr<<"Widget must remain a taskbar-owned popup\n";++failures;
        }
        int parent_changes=0,position_changes=0;
        std::pair<int*,int*> counters{&parent_changes,&position_changes};
        const SUBCLASSPROC count_changes=[](HWND w,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR context)->LRESULT{
            auto counters=reinterpret_cast<std::pair<int*,int*>*>(context);
            if(message==WM_STYLECHANGED)++*counters->first;
            if(message==WM_WINDOWPOSCHANGED)++*counters->second;
            return DefSubclassProc(w,message,wp,lp);
        };
        SetWindowSubclass(host.GetHwnd(),count_changes,99,reinterpret_cast<DWORD_PTR>(&counters));
        run_fullscreen_case([&](){
            pump();DwmFlush();GetWindowRect(input,&input_rect);
            if((GetWindowLongPtrW(host.GetHwnd(),GWL_STYLE)&WS_CHILD)||GetParent(host.GetHwnd())!=stable_parent){
                std::cerr<<"Shell panel changed the widget's popup surface\n";++failures;
            }
            // This unprivileged fixture cannot promote Explorer into Windows'
            // shell UI band. Verify attachment here; actual panel occlusion must
            // also be checked live against Explorer's real flyout.
            if(!host.IsRequestedVisible()||!host.IsVisible()||!IsWindowVisible(input)){
                std::cerr<<"Quick Settings shell surface hid the widget\n";++failures;
            }
        },L"--shell-helper");
        pump();
        if(GetParent(host.GetHwnd())!=stable_parent||parent_changes!=0){
            std::cerr<<"Shell panel caused a layered surface parent/style reset\n";++failures;
        }
        const int settled_positions=position_changes;
        pump();
        if(position_changes!=settled_positions){
            std::cerr<<"Idle widget repeatedly repairs its Z-order\n";++failures;
        }
        // Explorer may reorder unrelated taskbar children while no popup is
        // open. A nonoverlapping child must not trigger visual/input reorders.
        HWND unrelated=CreateWindowExW(WS_EX_NOACTIVATE,L"STATIC",L"Unrelated taskbar child",
            WS_CHILD|WS_VISIBLE,0,0,1,1,stable_parent,nullptr,GetModuleHandleW(nullptr),nullptr);
        pump();
        if(position_changes!=settled_positions){
            std::cerr<<"Unrelated shell child caused idle restacking\n";++failures;
        }
        DestroyWindow(unrelated);
        RemoveWindowSubclass(host.GetHwnd(),count_changes,99);

        host.Hide();
        host.UpdatePosition();
        pump();
        if (host.IsVisible() || IsWindowVisible(input)) {
            std::cerr << "Watchdog overrode explicit Hide\n"; ++failures;
        }
        host.SetOffset(0,0);host.Show();
        DestroyWindow(host.GetHwnd());
        host.UpdatePosition();
        if(!host.IsVisible()||!IsWindow(host.GetHwnd())){
            std::cerr<<"Widget did not recover a destroyed embedded surface\n";++failures;
        }
        host.Destroy();
        if (IsWindow(input)) ++failures;
    }
    CoUninitialize();
    if (failures) std::cerr << failures << " taskbar interaction failures\n";
    return failures ? 1 : 0;
}
