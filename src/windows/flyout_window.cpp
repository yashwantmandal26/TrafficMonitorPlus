// TrafficMonitorPlus - Interactive Quick Panel Dashboard Window Implementation
#include "pch.h"
#include "windows/flyout_window.h"
#include "utils/string_utils.h"
#include "rendering/dpi_helper.h"
#include <winsvc.h>

namespace tmp {

static constexpr const wchar_t* kFlyoutClassName = L"TrafficMonitorPlusFlyout";
static constexpr UINT kStorageReady = WM_APP + 72;

static bool MatchesNetworkProcess(HANDLE process, const std::wstring& path, uint64_t created) {
    wchar_t actual[32768]{};DWORD size=_countof(actual);
    FILETIME birth{},exit{},kernel{},user{};
    return process && created && QueryFullProcessImageNameW(process,0,actual,&size) &&
        _wcsicmp(actual,path.c_str())==0 && GetProcessTimes(process,&birth,&exit,&kernel,&user) &&
        ((uint64_t(birth.dwHighDateTime)<<32)|birth.dwLowDateTime)==created;
}

void FlyoutWindow::PrepareNetworkClose(const std::wstring& id) {
    m_pending_targets.clear();m_service_options.clear();m_service_picker=false;m_service_page=0;
    const auto entry=std::find_if(m_live_network.entries.begin(),m_live_network.entries.end(),[&](const auto& e){return e.id==id;});
    if(entry==m_live_network.entries.end() || !fs::path(id).is_absolute()) {m_notice=L"No verified running app is available.";return;}
    std::map<DWORD,uint64_t> active;
    for(const auto& p:entry->processes){
        HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,p.pid);
        if(MatchesNetworkProcess(process,id,p.created))active[p.pid]=p.created;
        if(process)CloseHandle(process);
    }
    if(active.empty()){m_notice=L"This app has exited or its identity changed. Wait for a fresh reading.";return;}
    // Resolve actual SCM service identities. Never terminate a shared service host.
    SC_HANDLE scm=OpenSCManagerW(nullptr,nullptr,SC_MANAGER_ENUMERATE_SERVICE);
    if(!scm){m_notice=L"Unable to verify service identities. Try running as administrator.";return;}
    DWORD needed=0,count=0,resume=0;
    EnumServicesStatusExW(scm,SC_ENUM_PROCESS_INFO,SERVICE_WIN32,SERVICE_ACTIVE,nullptr,0,&needed,&count,&resume,nullptr);
    std::vector<BYTE> buffer(needed);resume=0;
    const bool enumerated=needed&&EnumServicesStatusExW(scm,SC_ENUM_PROCESS_INFO,SERVICE_WIN32,SERVICE_ACTIVE,buffer.data(),DWORD(buffer.size()),&needed,&count,&resume,nullptr);
    CloseServiceHandle(scm);
    if(!enumerated){m_notice=L"Unable to read running services. No app was closed.";return;}
    const auto services=reinterpret_cast<const ENUM_SERVICE_STATUS_PROCESSW*>(buffer.data());
    for(DWORD i=0;i<count;++i){const auto& svc=services[i];
        const auto it=active.find(svc.ServiceStatusProcess.dwProcessId);
        if(it!=active.end())m_service_options.push_back({entry->name,svc.lpDisplayName,it->first,true,svc.lpServiceName,it->second,id});
    }
    if(!m_service_options.empty()){
        m_service_picker=true;m_confirm_active=true;m_focus=0;
        m_pending_kill={entry->name,entry->name,0,true,L""};return;
    }
    if(_wcsicmp(entry->name.c_str(),L"svchost.exe")==0){m_notice=L"No verified service can be stopped for this host.";return;}
    // App rows aggregate executable instances (e.g. a browser's network workers).
    // Freeze the exact path, PID and creation time of each instance for confirmation.
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    PROCESSENTRY32W pe{};pe.dwSize=sizeof(pe);
    if(snapshot!=INVALID_HANDLE_VALUE){
        if(Process32FirstW(snapshot,&pe))do{
            if(pe.th32ProcessID<=4 || pe.th32ProcessID==GetCurrentProcessId())continue;
            HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pe.th32ProcessID);
            wchar_t path[32768]{};DWORD length=_countof(path);FILETIME birth{},exit{},kernel{},user{};
            if(process&&QueryFullProcessImageNameW(process,0,path,&length)&&_wcsicmp(path,id.c_str())==0&&GetProcessTimes(process,&birth,&exit,&kernel,&user))
                m_pending_targets.push_back({pe.szExeFile,entry->name,pe.th32ProcessID,false,L"",(uint64_t(birth.dwHighDateTime)<<32)|birth.dwLowDateTime,id});
            if(process)CloseHandle(process);
        }while(Process32NextW(snapshot,&pe));CloseHandle(snapshot);
    }
    if(m_pending_targets.empty()){m_notice=L"No running app instances can be closed.";return;}
    m_pending_kill=m_pending_targets.front();m_confirm_active=true;m_focus=1;
}

FlyoutWindow::FlyoutWindow() {
    m_theme.Init(true); // Default dark Fluent appearance
}

FlyoutWindow::~FlyoutWindow() {
    if (m_storage_worker.valid()) {
        try { m_storage_worker.wait(); } catch (...) {}
    }
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

bool FlyoutWindow::Init(HINSTANCE hInstance) {
    if (m_hwnd) return true;
    m_hInstance = hInstance;

    if (!m_registered) {
        WNDCLASSEXW wc{};
        wc.cbSize        = sizeof(WNDCLASSEXW);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = FlyoutWindow::WndProc;
        wc.hInstance     = hInstance;
        wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr; // Direct2D paints
        wc.lpszClassName = kFlyoutClassName;

        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
        m_registered = true;
    }

    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        kFlyoutClassName,
        L"Quick Panel",
        WS_POPUP,
        100, 100, m_width, m_height,
        nullptr, nullptr, hInstance, this
    );

    if (!m_hwnd) return false;

    // Windows 11 Rounded Corners
    DWORD corner = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(m_hwnd, 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */, &corner, sizeof(corner));

    m_renderer = std::make_unique<D2DRenderer>();
    if (!m_renderer->Init(m_hwnd)) return false;
    m_renderer->SetFont(L"Segoe UI", 12.0f, false);

    return true;
}

void FlyoutWindow::Show(POINT anchor, bool above_anchor) {
    if (!m_hwnd) return;

    m_notice.clear();
    m_confirm_active = false;
    m_hover = m_focus = -1;
    if (network_usage_provider) {
        m_cached_net_usage = network_usage_provider(false);
        m_last_network_refresh = std::chrono::steady_clock::now();
    }
    // Scale dimensions based on current monitor DPI
    HMONITOR monitor = MonitorFromPoint(anchor, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{sizeof(mi)};
    GetMonitorInfoW(monitor, &mi);
    UINT dpi = DpiHelper::GetDpiForWindow(m_hwnd);
    HMODULE shcore = LoadLibraryW(L"shcore.dll");
    if (shcore) {
        using GetMonitorDpi = HRESULT(WINAPI*)(HMONITOR,int,UINT*,UINT*);
        auto getDpi = reinterpret_cast<GetMonitorDpi>(GetProcAddress(shcore,"GetDpiForMonitor"));
        UINT dy = 96; if (getDpi) getDpi(monitor, 0, &dpi, &dy);
        FreeLibrary(shcore);
    }
    float scale = static_cast<float>(dpi) / 96.0f;
    scale = (std::min)(scale, (std::min)((mi.rcWork.right - mi.rcWork.left - 16) / static_cast<float>(m_width),
                                        (mi.rcWork.bottom - mi.rcWork.top - 16) / static_cast<float>(m_height)));
    int scaled_w = static_cast<int>(m_width * scale);
    int scaled_h = static_cast<int>(m_height * scale);

    // Calculate position relative to taskbar anchor
    int x = anchor.x - (scaled_w / 2);
    int y = above_anchor ? (anchor.y - scaled_h - 12) : (anchor.y + 12);

    // Clamp to primary screen work area
    RECT rcWork = mi.rcWork;
    if (x < rcWork.left + 8) x = rcWork.left + 8;
    if (x + scaled_w > rcWork.right - 8) x = rcWork.right - scaled_w - 8;
    if (y < rcWork.top + 8) y = rcWork.top + 8;
    if (y + scaled_h > rcWork.bottom - 8) y = rcWork.bottom - scaled_h - 8;

    SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, scaled_w, scaled_h, SWP_SHOWWINDOW);
    SetForegroundWindow(m_hwnd);
    if (on_shown) on_shown();
    if (m_page == 2) {
        RefreshStorageAsync();
    }
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void FlyoutWindow::Hide() {
    if (m_hwnd && IsWindowVisible(m_hwnd)) {
        ShowWindow(m_hwnd, SW_HIDE);
        m_confirm_active = false;
        if (on_dismissed) on_dismissed();
    }
}

void FlyoutWindow::Toggle(POINT anchor, bool above_anchor) {
    if (IsVisible()) {
        Hide();
    } else {
        Show(anchor, above_anchor);
    }
}

bool FlyoutWindow::IsVisible() const {
    return (m_hwnd && IsWindowVisible(m_hwnd));
}

void FlyoutWindow::UpdateData(const TelemetryData& data, const std::vector<ProcessInfo>& top_processes, const std::wstring& local_ip) {
    m_latest_data = data;
    m_top_processes = top_processes;
    m_has_data = true;

    if (!local_ip.empty()) {
        m_cached_ip = local_ip;
    } else if (ip_provider) {
        m_cached_ip = ip_provider();
    }

    // Feed real-time sparklines
    m_net_sparkline.Push(data.download_bytes_per_sec);
    m_upload_sparkline.Push(data.upload_bytes_per_sec);
    m_cpu_sparkline.Push(data.cpu_usage_percent);
    m_ram_sparkline.Push(data.ram_usage_percent);

    auto now = std::chrono::steady_clock::now();
    if (IsVisible() && m_page == 0 && network_usage_provider && now - m_last_network_refresh >= std::chrono::seconds(15)) {
        m_cached_net_usage = network_usage_provider(false);
        m_last_network_refresh = now;
    }
    if (IsVisible() && m_page == 2 && now - m_last_storage_refresh >= std::chrono::seconds(5)) {
        RefreshStorageAsync();
    }

    if (IsVisible()) {
        NormalizePage();
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void FlyoutWindow::UpdateNetworkReport(const NetworkUsageReport& report) {
    m_cached_net_usage = report;
    if (IsVisible()) {
        NormalizePage();
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void FlyoutWindow::UpdateLiveNetwork(const LiveNetworkSnapshot& snapshot) {
    m_live_network = snapshot;
    if (IsVisible()) InvalidateRect(m_hwnd, nullptr, FALSE);
}

void FlyoutWindow::RefreshNetworkUsage() {
    if (on_retry_live_network) on_retry_live_network();
    if (network_usage_provider) {
        m_cached_net_usage = network_usage_provider(true);
        m_last_network_refresh = std::chrono::steady_clock::now();
        if (IsVisible()) {
            NormalizePage();
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }
}

bool FlyoutWindow::TerminateTarget(const KillTarget& target, std::wstring& out_message) {
    const auto label = target.display_name.empty() ? target.name : target.display_name;
    auto fail = [&](const std::wstring& reason) { out_message = reason; return false; };
    // SRUM identifiers may be paths/package IDs, never shell commands or service names.
    if (target.is_service) {
        if (target.service_name.empty()) return fail(L"No verified service identifier available.");
        SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!manager) return fail(L"Unable to access Service Manager.");
        SC_HANDLE service = OpenServiceW(manager, target.service_name.c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (!service) { CloseServiceHandle(manager); return fail(L"Service unavailable or access denied."); }
        if(target.created){
            SERVICE_STATUS_PROCESS current{};DWORD bytes=0;
            HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,target.pid);
            const bool same=QueryServiceStatusEx(service,SC_STATUS_PROCESS_INFO,reinterpret_cast<BYTE*>(&current),sizeof(current),&bytes)&&
                current.dwProcessId==target.pid&&MatchesNetworkProcess(process,target.executable_path,target.created);
            if(process)CloseHandle(process);
            if(!same){CloseServiceHandle(service);CloseServiceHandle(manager);return fail(L"Service identity changed. Refresh and try again.");}
        }
        SERVICE_STATUS status{};
        const bool sent = ControlService(service, SERVICE_CONTROL_STOP, &status) != FALSE;
        const DWORD error = sent ? ERROR_SUCCESS : GetLastError();
        CloseServiceHandle(service); CloseServiceHandle(manager);
        if (!sent) return fail(std::format(L"Service stop failed (Windows error {}).", error));
        out_message = L"Stop requested: " + label;
    } else {
        if (target.pid <= 4 || target.pid == GetCurrentProcessId()) return fail(L"Protected or unavailable process.");
        for (const auto* name : { L"csrss.exe", L"smss.exe", L"services.exe", L"lsass.exe", L"wininit.exe", L"winlogon.exe" })
            if (_wcsicmp(target.name.c_str(), name) == 0) return fail(L"Protected Windows process.");
        HANDLE process = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, target.pid);
        if (!process) return fail(L"Process has exited or access was denied.");
        wchar_t path[32768]{}; DWORD length = _countof(path); BOOL critical = FALSE;
        const bool verified = QueryFullProcessImageNameW(process, 0, path, &length) &&
            _wcsicmp(fs::path(path).filename().c_str(), fs::path(target.name).filename().c_str()) == 0 &&
            (target.executable_path.empty() || MatchesNetworkProcess(process,target.executable_path,target.created)) &&
            IsProcessCritical(process, &critical) && !critical;
        if (!verified) { CloseHandle(process); return fail(L"Process identity changed or process is protected."); }
        const bool killed = TerminateProcess(process, 1) != FALSE;
        const DWORD error = killed ? ERROR_SUCCESS : GetLastError();
        CloseHandle(process);
        if (!killed) return fail(std::format(L"Close failed (Windows error {}).", error));
        std::erase_if(m_top_processes, [&](const ProcessInfo& p) { return p.pid == target.pid; });
        out_message = L"Close requested: " + label;
    }
    if (on_kill_target) on_kill_target(target.name, target.pid, target.is_service);
    return true;
}

std::vector<DiskInfo> FlyoutWindow::SampleDrives() {
    if (storage_provider) return storage_provider();
    std::vector<DiskInfo> drives;
    std::set<std::wstring> volumes;
    const DWORD driveMask = GetLogicalDrives();
    for (char c = 'A'; c <= 'Z'; ++c) {
        if (driveMask & (1 << (c - 'A'))) {
            wchar_t root[] = { (wchar_t)c, L':', L'\\', 0 };
            const UINT type = GetDriveTypeW(root);
            if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE) {
                ULARGE_INTEGER freeCaller{}, totalBytes{}, freeBytes{};
                if (GetDiskFreeSpaceExW(root, &freeCaller, &totalBytes, &freeBytes) && totalBytes.QuadPart > 0) {
                    if (freeCaller.QuadPart > totalBytes.QuadPart) continue;
                    wchar_t volume[MAX_PATH]{};
                    if (GetVolumeNameForVolumeMountPointW(root, volume, MAX_PATH) && !volumes.insert(volume).second) continue;
                    DiskInfo d{};
                    d.letter = (wchar_t)c;
                    d.total = totalBytes.QuadPart;
                    d.free = freeCaller.QuadPart;
                    d.used = (d.total > d.free) ? (d.total - d.free) : 0;
                    d.used_pct = (static_cast<double>(d.used) / static_cast<double>(d.total)) * 100.0;
                    wchar_t vol[MAX_PATH + 1]{};
                    if (GetVolumeInformationW(root, vol, MAX_PATH + 1, nullptr, nullptr, nullptr, nullptr, 0) && vol[0] != 0) {
                        d.label = vol;
                    } else {
                        d.label = (type == DRIVE_REMOVABLE) ? L"Removable" : L"Local Disk";
                    }
                    drives.push_back(std::move(d));
                }
            }
        }
    }
    return drives;
}

void FlyoutWindow::RefreshStorageSnapshot() {
    if (m_storage_refreshing) return;
    m_cached_drives = SampleDrives();
    m_last_storage_refresh = std::chrono::steady_clock::now();
}

void FlyoutWindow::RefreshStorageAsync() {
    if (m_storage_refreshing) return;
    m_storage_refreshing = true;
    m_last_storage_refresh = std::chrono::steady_clock::now();
    m_storage_worker = std::async(std::launch::async, [this]() {
        std::vector<DiskInfo> drives;
        try { drives = SampleDrives(); }
        catch (...) { PostMessageW(m_hwnd, kStorageReady, 0, 0); throw; }
        PostMessageW(m_hwnd, kStorageReady, 0, 0);
        return drives;
    });
}

void FlyoutWindow::AcceptStorageSnapshot() {
    if (!m_storage_refreshing || !m_storage_worker.valid()) return;
    try {
        m_cached_drives = m_storage_worker.get();
        m_last_storage_refresh = std::chrono::steady_clock::now();
    } catch (...) {
        m_notice = L"Storage refresh failed. Try Refresh again.";
    }
    m_storage_refreshing = false;
    m_focus = -1;
    NormalizePage();
    if (m_notice == L"Refreshing storage...") m_notice = L"Storage updated.";
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

LRESULT CALLBACK FlyoutWindow::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    FlyoutWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = reinterpret_cast<FlyoutWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<FlyoutWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (!self) return DefWindowProcW(hwnd, msg, wp, lp);

    switch (msg) {
        case kStorageReady:
            self->AcceptStorageSnapshot();
            return 0;

        case WM_NCHITTEST: {
            POINT point{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
            ScreenToClient(hwnd, &point);
            const float scale = self->CanvasScale();
            if (point.y / scale < 68 && self->ControlAt(point.x / scale, point.y / scale) < 0) return HTCAPTION;
            return HTCLIENT;
        }

        case WM_DPICHANGED: {
            auto rect = reinterpret_cast<RECT*>(lp);
            SetWindowPos(hwnd, nullptr, rect->left, rect->top, rect->right - rect->left,
                rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            self->Render();
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_ACTIVATE:
            if (LOWORD(wp) == WA_INACTIVE && !self->m_pinned) {
                HWND next = reinterpret_cast<HWND>(lp);
                if (!next || GetWindow(next, GW_OWNER) != hwnd) self->Hide();
            }
            return 0;

        case WM_LBUTTONDOWN: {
            const float scale=self->CanvasScale();
            const float x=GET_X_LPARAM(lp)/scale,y=GET_Y_LPARAM(lp)/scale;
            if(!self->m_confirm_active && x>=458 && x<=474 && y>=(self->m_page==0?198:160) && y<=546 && self->MaxScroll()>0){
                const float height=self->m_page==0?348.0f:386.0f;
                const float thumb=(std::max)(32.0f,height*height/self->ContentExtent());
                const float top=(self->m_page==0?198:160)+(height-thumb)*self->m_scroll/self->MaxScroll();
                self->m_scroll_grab=y>=top&&y<=top+thumb?y-top:thumb/2;
                self->m_scroll_dragging=true;SetCapture(hwnd);self->DragScrollbar(y);return 0;
            }
            break;
        }
        case WM_CAPTURECHANGED:
            self->m_scroll_dragging=false;return 0;
        case WM_LBUTTONUP:
            if(self->m_scroll_dragging){self->m_scroll_dragging=false;ReleaseCapture();return 0;}
            self->OnLButtonUp(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            return 0;

        case WM_MOUSEMOVE: {
            const float scale = self->CanvasScale();
            if(self->m_scroll_dragging){self->DragScrollbar(GET_Y_LPARAM(lp)/scale);return 0;}
            const int hover = self->ControlAt(GET_X_LPARAM(lp) / scale, GET_Y_LPARAM(lp) / scale);
            if (hover != self->m_hover) {
                self->m_hover = hover;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            SetCursor(LoadCursorW(nullptr, hover >= 0 ? IDC_HAND : IDC_ARROW));
            if (!self->m_tracking) {
                TRACKMOUSEEVENT track{ sizeof(track), TME_LEAVE, hwnd, 0 };
                TrackMouseEvent(&track);
                self->m_tracking = true;
            }
            return 0;
        }

        case WM_MOUSELEAVE:
            self->m_tracking = false;
            self->m_hover = -1;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_MOUSEWHEEL:
            self->m_wheel_remainder+=GET_WHEEL_DELTA_WPARAM(wp);
            while(std::abs(self->m_wheel_remainder)>=WHEEL_DELTA){
                const int direction=self->m_wheel_remainder>0?-1:1;
                self->Scroll(direction);self->m_wheel_remainder+=direction*WHEEL_DELTA;
            }
            return 0;

        case WM_KEYDOWN:
            if (wp == VK_ESCAPE) {
                if (self->m_confirm_active) {
                    self->m_confirm_active = false;
                    self->m_focus = -1;
                    self->NormalizePage();
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else self->Hide();
                return 0;
            }
            if (self->m_confirm_active && wp != VK_TAB && wp != VK_RETURN && wp != VK_SPACE) return 0;
            if (GetKeyState(VK_CONTROL) < 0 && wp >= '1' && wp <= '3') {
                self->ChangePage(static_cast<int>(wp - '1'));
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            if (wp == VK_TAB && !self->m_controls.empty()) {
                const int count = static_cast<int>(self->m_controls.size());
                const bool back = GetKeyState(VK_SHIFT) < 0;
                self->m_focus = self->m_focus < 0 ? (back ? count - 1 : 0) : (self->m_focus + (back ? count - 1 : 1)) % count;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            if ((wp == VK_RETURN || wp == VK_SPACE) && self->m_focus >= 0 && self->m_focus < static_cast<int>(self->m_controls.size())) {
                self->ActivateControl(self->m_controls[self->m_focus].id);
                return 0;
            }
            if(wp==VK_HOME||wp==VK_END){self->m_scroll=wp==VK_HOME?0:self->MaxScroll();self->m_focus=-1;InvalidateRect(hwnd,nullptr,FALSE);return 0;}
            if(wp==VK_NEXT||wp==VK_PRIOR){self->m_scroll=std::clamp(self->m_scroll+(wp==VK_NEXT?320:-320),0,self->MaxScroll());self->m_focus=-1;InvalidateRect(hwnd,nullptr,FALSE);return 0;}
            if(wp==VK_DOWN||wp==VK_UP){self->Scroll(wp==VK_DOWN?1:-1);return 0;}
            break;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

float FlyoutWindow::CanvasScale() const {
    RECT r{};
    GetClientRect(m_hwnd, &r);
    return (std::max)(0.01f, (std::min)(r.right / static_cast<float>(m_width), r.bottom / static_cast<float>(m_height)));
}

int FlyoutWindow::ControlAt(float x, float y) const {
    for (const auto& c : m_controls) {
        if (x >= c.rect.left && x < c.rect.right && y >= c.rect.top && y < c.rect.bottom) return c.id;
    }
    return -1;
}

void FlyoutWindow::ActivateControl(int control) {
    if (m_confirm_active) {
        if(m_service_picker){
            if(control==51){m_confirm_active=false;m_service_picker=false;m_focus=-1;}
            else if(control==80)m_service_page=(std::max)(0,m_service_page-1);
            else if(control==81)m_service_page=(std::min)(int((m_service_options.size()-1)/4),m_service_page+1);
            else if(control>=60&&control<64){
                const size_t index=m_service_page*4+control-60;
                if(index<m_service_options.size()){m_pending_targets={m_service_options[index]};m_pending_kill=m_pending_targets.front();m_service_picker=false;m_focus=1;}
            }
            InvalidateRect(m_hwnd,nullptr,FALSE);return;
        }
        if (control == 50) { // Confirm termination
            std::wstring feedback;
            if(m_pending_targets.empty())m_pending_targets={m_pending_kill};
            size_t succeeded=0;std::wstring failure;
            for(const auto& target:m_pending_targets){if(TerminateTarget(target,feedback))++succeeded;else failure=feedback;}
            m_notice=m_pending_targets.size()==1?feedback:std::format(L"Closed {} / {} app processes. {}",succeeded,m_pending_targets.size(),failure);
            m_confirm_active = false;
            m_focus = -1;
            NormalizePage();
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return;
        } else if (control == 51) { // Cancel termination
            m_confirm_active = false;
            m_focus = -1;
            m_notice = L"Termination cancelled.";
            NormalizePage();
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return;
        } else if (control == 3) {
            m_confirm_active = false;
            Hide();
            return;
        }
        return; // Ignore other controls during confirmation
    }

    if (control == 3) { Hide(); return; }
    if (control == 0 && on_copy_ip) m_notice = on_copy_ip() ? L"IP address copied" : L"IP address unavailable";
    else if (control == 1 && on_data_usage) on_data_usage();
    else if (control == 2 && on_open_task_manager) on_open_task_manager();
    else if (control >= 4 && control <= 6) ChangePage(control - 4);
    else if (control == 7) m_pinned = !m_pinned;
    else if (control == 8 && on_open_settings) on_open_settings();
    else if (control == 9 || control == 10) { m_sort_memory = (control == 10); m_scroll = 0; m_focus = -1; }
    else if (control == 11 || control == 12) Scroll(control == 11 ? -1 : 1);
    else if (control == 13) { RefreshStorageAsync(); m_notice = L"Refreshing storage..."; }
    else if (control == 14) ChangePage(1);
    else if (control == 15) RefreshNetworkUsage();
    else if (control >= 20 && control <= 22) { m_quick_filter = control - 20; m_scroll = 0; m_focus = -1; }
    else if (control >= 100 && control < 126 && on_open_drive) on_open_drive(static_cast<wchar_t>(L'A' + control - 100));
    else if (control >= 200) {
        for (const auto& ka : m_kill_actions) {
            if (ka.control_id == control) {
                if(!ka.target.executable_path.empty() && !ka.target.created){PrepareNetworkClose(ka.target.executable_path);break;}
                m_pending_targets.clear();m_service_picker=false;
                m_confirm_active = true;
                m_focus = 1; // Cancel is the initial keyboard action.
                m_pending_kill = ka.target;
                InvalidateRect(m_hwnd, nullptr, FALSE);
                break;
            }
        }
    }
    NormalizePage();
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void FlyoutWindow::ChangePage(int page) {
    const bool opening_storage = (page == 2 && m_page != 2);
    m_page = page;
    m_scroll = 0;
    m_hover = m_focus = -1;
    m_confirm_active = false;
    m_notice.clear();
    if (opening_storage && IsVisible()) RefreshStorageAsync();
    NormalizePage();
}

void FlyoutWindow::Scroll(int direction) {
    if (m_confirm_active) return;
    m_scroll=std::clamp(m_scroll+direction*64,0,MaxScroll());
    m_hover = m_focus = -1;
    NormalizePage();
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void FlyoutWindow::OnLButtonUp(int x, int y) {
    const float scale = CanvasScale();
    ActivateControl(ControlAt(x / scale, y / scale));
}

int FlyoutWindow::ContentExtent() const {
    if(m_page==1)return 68+(std::max)(1,int(m_top_processes.size()))*48;
    if(m_page==2)return 52+(std::max)(1,int(m_cached_drives.size()))*76;
    return m_quick_filter==0?674:332;
}
int FlyoutWindow::MaxScroll() const {return (std::max)(0,ContentExtent()-(m_page==0?348:386));}
void FlyoutWindow::NormalizePage() {m_scroll=std::clamp(m_scroll,0,MaxScroll());}
void FlyoutWindow::DragScrollbar(float y) {
    const float height=m_page==0?348.0f:386.0f;
    const float thumb=(std::max)(32.0f,height*height/ContentExtent());
    m_scroll=int(std::clamp((y-(m_page==0?198:160)-m_scroll_grab)/(height-thumb),0.0f,1.0f)*MaxScroll());
    m_hover=m_focus=-1;InvalidateRect(m_hwnd,nullptr,FALSE);
}

void FlyoutWindow::Render() {
    if (!m_renderer) return;
    RECT r{}; GetClientRect(m_hwnd, &r);
    if (r.right <= 0 || r.bottom <= 0) return;
    m_renderer->Resize(r.right, r.bottom);
    auto c = [](unsigned rgb, float a = 1.0f) { return D2D1::ColorF(rgb, a); };
    const auto ink=c(0xF1F3F4), muted=c(0xADB9BC), mint=c(0x92E8C0), cyan=c(0x81CBEB), amber=c(0xF3C27A), coral=c(0xEF8D8D);
    if (!m_renderer->BeginDraw(c(0x101718))) return;
    auto* target=m_renderer->GetTarget(); if (!target) return;
    float dx=96,dy=96; target->GetDpi(&dx,&dy);
    target->SetTransform(D2D1::Matrix3x2F::Scale(CanvasScale()/(dx/96),CanvasScale()/(dy/96)));
    m_controls.clear(); m_kill_actions.clear();
    auto text = [&](std::wstring value,float x,float y,float width,float size,D2D1_COLOR_F color,bool bold=false) {
        m_renderer->SetFont(L"Segoe UI",size,bold);
        if (m_renderer->MeasureText(value).width>width) {
            while (!value.empty() && m_renderer->MeasureText(value+L"\u2026").width>width) value.pop_back();
            value+=L"\u2026";
        }
        m_renderer->DrawText(value,D2D1::RectF(x,y,x+width,y+size*1.55f+2),color);
    };
    auto right = [&](const std::wstring& value,float x,float y,float width,float size,D2D1_COLOR_F color,bool bold=false) {
        m_renderer->SetFont(L"Segoe UI",size,bold);
        const float w=(std::min)(width,m_renderer->MeasureText(value).width);
        text(value,x-w,y,w+1,size,color,bold);
    };
    auto surface = [&](D2D1_RECT_F rect,unsigned color=0x1C282B) { m_renderer->FillRoundedRect(rect,7,c(color)); };
    bool body=false;
    auto reg = [&](D2D1_RECT_F rect,int id) {
        const auto drawRect=rect;
        if(body){
            rect.left=18+(rect.left-18)*.965f;rect.right=18+(rect.right-18)*.965f;
            rect.top-=m_scroll;rect.bottom-=m_scroll;
            rect.top=(std::max)(m_page==0?198.0f:160.0f,rect.top);rect.bottom=(std::min)(546.0f,rect.bottom);
            if(rect.bottom<=rect.top)return;
        }
        if (m_focus==static_cast<int>(m_controls.size())) m_renderer->DrawRoundedRect(drawRect,6,mint,1.5f);
        m_controls.push_back({rect,id});
    };
    auto button = [&](D2D1_RECT_F rect,int id,const std::wstring& label,bool selected=false,bool enabled=true) {
        surface(rect,selected?0xCEECD9:(m_hover==id?0x34474A:0x243235));
        m_renderer->SetFont(L"Segoe UI",13,true);
        auto shown=label;
        const float available=rect.right-rect.left-12;
        if(m_renderer->MeasureText(shown).width>available){
            while(!shown.empty()&&m_renderer->MeasureText(shown+L"\u2026").width>available)shown.pop_back();
            shown+=L"\u2026";
        }
        const float w=m_renderer->MeasureText(shown).width;
        text(shown,rect.left+(rect.right-rect.left-w)/2,rect.top+(rect.bottom-rect.top-21)/2,w+2,13,
            !enabled?c(0x748689):(selected?c(0x142D23):ink),true);
        if(enabled) reg(rect,id);
    };
    auto meter = [&](float x,float y,float width,double percent,D2D1_COLOR_F color) {
        m_renderer->FillRoundedRect(D2D1::RectF(x,y,x+width,y+3),1.5f,c(0x354649));
        const float fill=width*float(std::clamp(std::isfinite(percent)?percent:0.0,0.0,100.0))/100;
        if(fill>0) m_renderer->FillRoundedRect(D2D1::RectF(x,y,x+fill,y+3),1.5f,color);
    };
    m_renderer->DrawRoundedRect(D2D1::RectF(.5f,.5f,479.5f,float(m_height)-.5f),14,c(0x42585B),1);
    text(L"Quick Panel",18,12,270,26,ink,true);
    button(D2D1::RectF(352,15,416,47),7,m_pinned?L"Pinned":L"Pin",m_pinned);
    button(D2D1::RectF(424,15,462,47),3,L"\u00D7");
    const auto minutes=GetTickCount64()/60000;
    const auto ip=m_cached_ip.empty()||m_cached_ip==L"0.0.0.0"?L"IP unavailable":m_cached_ip;
    text(std::format(L"Uptime {}h {}m  \u00B7  {}",minutes/60,minutes%60,ip),20,48,350,11.5f,muted);
    const bool stale=m_has_data && (m_latest_data.timestamp.time_since_epoch().count()==0 ||
        std::chrono::steady_clock::now()-m_latest_data.timestamp>std::chrono::seconds(10));
    right(stale?L"STALE":m_has_data?L"LIVE":L"WAIT",460,49,70,11,stale?amber:mint,true);
    const auto& d=m_latest_data;
    surface(D2D1::RectF(18,74,462,110));
    text(L"\u2193 "+FormatSpeed(d.download_bytes_per_sec),27,82,104,13,mint,true);
    text(L"\u2191 "+FormatSpeed(d.upload_bytes_per_sec),140,82,104,13,cyan,true);
    text(L"CPU "+FormatPercent(d.cpu_usage_percent),254,82,98,13,ink,true);
    text(L"RAM "+FormatPercent(d.ram_usage_percent),362,82,95,13,ink,true);
    const wchar_t* tabs[]={L"Overview",L"Processes",L"Storage"};
    for(int i=0;i<3;++i) button(D2D1::RectF(18+i*150.0f,118,162+i*150.0f,150),4+i,tabs[i],m_page==i);
    auto killButton = [&](float y,const KillTarget& kt) {
        if(!kt.pid || kt.pid==GetCurrentProcessId()) return;
        const int id=200+static_cast<int>(m_kill_actions.size());
        button(D2D1::RectF(428,y+2,458,y+28),id,L"\u00D7");
        m_kill_actions.push_back({id,kt});
    };
    auto hogRow = [&](float y,const std::wstring& name,const std::wstring& tag,const std::wstring& metric,D2D1_COLOR_F color,const KillTarget& kt,bool details=false,double percent=0.0) {
        surface(D2D1::RectF(18,y,462,y+(details?40:28)),0x1B2528);
        text(tag,25,y+5,32,10,color,true);
        text(name,62,y+3,238,13,ink,true);
        right(metric,kt.pid?420.0f:452.0f,y+4,112,12.5f,color,true);
        killButton(y,kt);
        if(details){text(std::format(L"PID {}",kt.pid),62,y+21,185,10.5f,muted);meter(258,y+32,160,percent,color);}
    };
    NormalizePage();
    float y=158;
    if(m_page==0){
        button(D2D1::RectF(18,158,158,190),20,L"All activity",m_quick_filter==0);
        button(D2D1::RectF(166,158,306,190),21,L"Network",m_quick_filter==1);
        button(D2D1::RectF(314,158,462,190),22,L"CPU & RAM",m_quick_filter==2);
    }
    // One clipped content viewport, independently scrollable below the navigation.
    const float viewportTop=m_page==0?198.0f:160.0f;
    target->PushAxisAlignedClip(D2D1::RectF(18,viewportTop,450,546),D2D1_ANTIALIAS_MODE_ALIASED);
    const auto base= D2D1::Matrix3x2F::Scale(CanvasScale()/(dx/96),CanvasScale()/(dy/96));
    target->SetTransform(D2D1::Matrix3x2F::Scale(.965f,1,D2D1::Point2F(18,0))*D2D1::Matrix3x2F::Translation(0,float(-m_scroll))*base);
    body=true;
    if(m_page==0){
        if(m_quick_filter!=2){
            const auto& live=m_live_network;
            const bool fresh=live.state==LiveNetworkState::Live && live.sampled_at.time_since_epoch().count()!=0 &&
                std::chrono::steady_clock::now()-live.sampled_at<std::chrono::seconds(5);
            text(live.message,20,196,350,12,live.state==LiveNetworkState::Live?mint:amber,true);
            text(live.lost_events?L"Some traffic events were lost; totals are partial.":L"Per-app speed (B/s) \u00B7 updated every second",20,215,350,11.5f,live.lost_events?amber:muted);
            button(D2D1::RectF(378,195,462,225),15,L"Refresh");
            for(int kind=0;kind<2;++kind) {
                const float left=kind?244.0f:18.0f, edge=left+218;
                const auto accent=kind?cyan:mint;
                surface(D2D1::RectF(left,232,edge,396),0x172326);
                text(kind?L"Downloading now":L"Uploading now",left+8,239,202,11.5f,accent,true);
                auto entries=live.entries;
                auto rate=[&](const LiveNetworkEntry& e){return kind?e.download_bps:e.upload_bps;};
                std::sort(entries.begin(),entries.end(),[&](const auto& a,const auto& b){if(rate(a)!=rate(b))return rate(a)>rate(b);return a.id<b.id;});
                int count=0;
                if(fresh)for(const auto& entry:entries){
                    if(count==3||rate(entry)<=0)break;
                    const float row=262+count*44.0f;
                    surface(D2D1::RectF(left+5,row,edge-5,row+40),0x213135);
                    text(entry.name,left+11,row+2,156,13,ink,true);
                    text(FormatSpeed(rate(entry)),left+11,row+21,156,13,accent,true);
                    const int closeId=200+int(m_kill_actions.size());
                    const bool identified=!entry.processes.empty()&&fs::path(entry.id).is_absolute();
                    button(D2D1::RectF(edge-37,row+6,edge-9,row+34),closeId,L"\u00D7",false,identified);
                    m_kill_actions.push_back({closeId,{entry.name,entry.name,0,false,L"",0,entry.id}});
                    ++count;
                }
                if(!count){
                    text(live.state==LiveNetworkState::Live?(kind?L"No active downloads":L"No active uploads"):L"Live capture unavailable",left+10,272,198,13,muted,true);
                    text(L"Adapter history \u00B7 last 60s",left+10,304,198,11,muted);
                    const auto& graph=kind?m_net_sparkline:m_upload_sparkline;
                    graph.Render(m_renderer.get(),D2D1::RectF(left+10,328,edge-10,384),accent,c(kind?0x81CBEB:0x92E8C0,.10f));
                }
            }
            const auto& report=m_cached_net_usage;
            const bool native=report.ready&&report.status!=ReportStatus::Failed&&!report.attribution_failed&&report.total_download+report.total_upload>0&&!report.items.empty();
            text(native?L"Data used today \u00B7 Windows records":L"Data used today \u00B7 tracked since "+(live.coverage_start.empty()?L"START":live.coverage_start),20,406,440,12,amber,true);
            struct DailyRow {std::wstring name;uint64_t bytes;std::wstring tag;};
            std::vector<DailyRow> daily;
            if(native){for(const auto& e:report.items)daily.push_back({e.display_name,e.Combined(),e.id==L"__OS_UNATTRIBUTED__"||e.id==L"__ATTR_FAILED__"?L"N/A":e.category==AppUsageCategory::Service?L"SVC":L"APP"});}
            else {for(const auto& e:live.entries)daily.push_back({e.name,e.today_download+e.today_upload,e.name==L"System"?L"SYS":e.id.starts_with(L"pid:")?L"PID":_wcsicmp(e.name.c_str(),L"svchost.exe")==0?L"HOST":L"APP"});}
            std::sort(daily.begin(),daily.end(),[](const auto& a,const auto& b){if(a.bytes!=b.bytes)return a.bytes>b.bytes;return a.name<b.name;});
            int count=0;for(const auto& e:daily){if(count==3||!e.bytes)break;hogRow(432+count*30.0f,e.name,e.tag,FormatMemory(e.bytes),amber,{});++count;}
            if(!count){
                surface(D2D1::RectF(18,432,462,522),0x172326);
                text(L"Daily app totals appear as traffic is captured.",28,442,424,13,muted);
                text(L"Adapter since app start: \u2193 "+FormatMemory(d.total_session_download)+L"  \u2191 "+FormatMemory(d.total_session_upload),28,470,424,13,ink,true);
                text(L"Traffic before tracking started is not included.",28,498,424,11.5f,muted);
            }

        }
        if(m_quick_filter!=1){
            const float systemTop=m_quick_filter==0?548.0f:206.0f;
            y=systemTop;
            for(int kind=0;kind<2;++kind) {
                auto processes=m_top_processes;
                std::sort(processes.begin(),processes.end(),[&](const auto& a,const auto& b){
                    if(kind==1&&a.memory_bytes!=b.memory_bytes) return a.memory_bytes>b.memory_bytes;
                    if(a.cpu_percent!=b.cpu_percent) return a.cpu_percent>b.cpu_percent;
                    if(a.memory_bytes!=b.memory_bytes) return a.memory_bytes>b.memory_bytes;
                    return a.pid<b.pid;
                });
                text(kind?L"Most memory used":L"Highest CPU usage",20,y,420,12,kind?cyan:mint,true);y+=24;
                for(size_t i=0;i<(std::min)(size_t{3},processes.size());++i) {
                    const auto& p=processes[i];
                    hogRow(y,p.name,kind?L"RAM":L"CPU",kind?FormatMemory(p.memory_bytes):FormatPercent(p.cpu_percent),kind?cyan:mint,{p.name,p.name,p.pid,false,L""},true,kind?(d.ram_total_bytes?100.0*p.memory_bytes/d.ram_total_bytes:0):p.cpu_percent);
                    y+=46;
                }
                if(processes.empty()) {surface(D2D1::RectF(18,y,462,y+132));text(L"Waiting for a process snapshot\u2026",24,y+20,430,13,muted);}
                y=systemTop+(kind?324.0f:162.0f);
            }

        }
    } else if(m_page==1){
        button(D2D1::RectF(18,160,154,192),9,L"Highest CPU",!m_sort_memory);
        button(D2D1::RectF(162,160,320,192),10,L"Most memory",m_sort_memory);
        right(std::format(L"{} apps",m_top_processes.size()),462,168,130,12,muted);
        text(L"APPLICATION / PID",24,202,240,11,muted,true);
        right(L"CPU",337,202,64,11,muted,true);right(L"MEMORY",420,202,80,11,muted,true);
        auto processes=m_top_processes;
        std::sort(processes.begin(),processes.end(),[&](const auto& a,const auto& b){
            if(m_sort_memory&&a.memory_bytes!=b.memory_bytes)return a.memory_bytes>b.memory_bytes;
            if(a.cpu_percent!=b.cpu_percent)return a.cpu_percent>b.cpu_percent;
            if(a.memory_bytes!=b.memory_bytes)return a.memory_bytes>b.memory_bytes;
            return a.pid<b.pid;
        });
        y=224;
        for(const auto& p:processes){
            surface(D2D1::RectF(18,y,462,y+44));
            text(p.name,24,y+2,240,13,ink,true);text(std::format(L"PID {}",p.pid),24,y+23,240,10.5f,muted);
            right(FormatPercent(p.cpu_percent),337,y+10,64,12.5f,mint,true);
            right(p.memory_bytes?FormatMemory(p.memory_bytes):L"\u2014",420,y+10,80,12.5f,ink);
            killButton(y+5,{p.name,p.name,p.pid,false,L""});y+=48;
        }
        if(processes.empty())text(L"Waiting for running applications...",24,250,420,14,muted);
    } else {
        uint64_t free=0,total=0;for(const auto& disk:m_cached_drives){free+=disk.free;total+=disk.total;}
        text(FormatMemory(free)+L" available",20,160,330,18,ink,true);
        text(std::format(L"{} drives \u00B7 select a drive to open",m_cached_drives.size()),20,185,330,11.5f,muted);
        button(D2D1::RectF(366,160,462,192),13,m_storage_refreshing?L"Reading...":L"Refresh",false,!m_storage_refreshing);
        y=212;
        for(const auto& disk:m_cached_drives){
            const int id=100+disk.letter-L'A';
            surface(D2D1::RectF(18,y,462,y+70),m_hover==id?0x2B4141:0x1C282B);
            text(std::wstring(1,disk.letter)+L":",27,y+12,40,20,mint,true);
            text(disk.label,76,y+3,270,14,ink,true);
            right(disk.used_pct>=90?L"LOW SPACE":L"Open",452,y+6,100,11.5f,disk.used_pct>=90?amber:muted,true);
            text(std::format(L"{} free of {} \u00B7 {:.0f}% used",FormatMemory(disk.free),FormatMemory(disk.total),disk.used_pct),76,y+25,376,12,muted);
            meter(76,y+57,374,disk.used_pct,disk.used_pct>=90?amber:mint);
            if(disk.letter>=L'A'&&disk.letter<=L'Z')reg(D2D1::RectF(18,y,462,y+70),id);
            y+=76;
        }
        if(m_cached_drives.empty())text(m_storage_refreshing?L"Reading drives...":L"No accessible drives. Try Refresh.",24,245,420,14,muted);
    }
    body=false;target->SetTransform(base);target->PopAxisAlignedClip();
    const float trackTop=m_page==0?198.0f:160.0f, trackHeight=546-trackTop;
    surface(D2D1::RectF(458,trackTop,474,546),0x1A282B);
    const float thumb=(std::max)(32.0f,trackHeight*trackHeight/ContentExtent());
    const float thumbHeight=(std::min)(trackHeight,thumb);
    const float thumbTop=trackTop+(MaxScroll()?m_scroll/float(MaxScroll()):0)*(trackHeight-thumbHeight);
    surface(D2D1::RectF(461,thumbTop,471,thumbTop+thumbHeight),MaxScroll()?0x91B6AD:0x425652);
    // Fixed footer and fixed frame: filters, empty states and pagination never resize the panel.
    y=552;
    m_renderer->DrawLine(D2D1::Point2F(18,y+2),D2D1::Point2F(462,y+2),c(0x354649),1);
    button(D2D1::RectF(18,y+10,122,y+46),0,L"Copy IP");
    button(D2D1::RectF(130,y+10,234,y+46),1,L"Data usage");
    button(D2D1::RectF(242,y+10,352,y+46),2,L"Task Mgr");
    button(D2D1::RectF(360,y+10,462,y+46),8,L"Settings");
    text(!m_notice.empty()?m_notice:m_pinned?L"Pinned \u00B7 stays open when focus moves away":L"Quick Panel \u00B7 Esc to close",20,y+54,440,11.5f,m_notice.empty()?muted:mint);
    if(m_confirm_active){
        // Modal hit targets replace background controls, including keyboard navigation.
        m_controls.clear();
        surface(D2D1::RectF(8,8,472,float(m_height)-8),0x11191C);
        if(m_service_picker){
            text(L"Choose a service to stop",30,120,420,22,ink,true);
            text(m_pending_kill.display_name,30,157,420,15,mint,true);
            text(L"This host may run several services. Select one.",30,187,420,13,muted);
            for(int i=0;i<4;++i){const size_t index=m_service_page*4+i;if(index>=m_service_options.size())break;
                const auto& option=m_service_options[index];
                button(D2D1::RectF(30,224+i*44.0f,450,262+i*44.0f),60+i,option.display_name);
            }
            const int pages=(std::max)(1,int((m_service_options.size()+3)/4));
            button(D2D1::RectF(30,412,132,448),80,L"Previous",false,m_service_page>0);
            text(std::format(L"{} / {}",m_service_page+1,pages),210,420,90,13,muted);
            button(D2D1::RectF(348,412,450,448),81,L"Next",false,m_service_page+1<pages);
            button(D2D1::RectF(248,472,450,514),51,L"Cancel",true);
            target->SetTransform(D2D1::Matrix3x2F::Identity());m_renderer->EndDraw();return;
        }
        const float top=(m_height-280)/2.0f;
        text(m_pending_kill.is_service?L"Stop this service?":L"Close this process?",30,top,420,22,ink,true);
        text(m_pending_kill.display_name.empty()?m_pending_kill.name:m_pending_kill.display_name,30,top+44,420,16,mint,true);
        text(m_pending_targets.size()>1?std::format(L"{} processes of this app will be closed",m_pending_targets.size()):m_pending_kill.is_service?L"Service: "+m_pending_kill.service_name:std::format(L"PID {}",m_pending_kill.pid),30,top+76,420,13,muted);
        text(L"Unsaved work in this application may be lost.",30,top+116,420,14,amber);
        text(L"Confirm to request termination.",30,top+144,420,14,ink);
        button(D2D1::RectF(30,top+198,232,top+240),50,m_pending_kill.is_service?L"Stop service":L"Force Close");
        button(D2D1::RectF(248,top+198,450,top+240),51,L"Cancel",true);
    }
    target->SetTransform(D2D1::Matrix3x2F::Identity());m_renderer->EndDraw();
}

} // namespace tmp
