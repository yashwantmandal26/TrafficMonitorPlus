// TrafficMonitorPlus - System Tray Notification Icon Implementation
#include "pch.h"
#include "windows/tray_icon.h"

namespace tmp {
namespace {
struct MenuMetric { DisplayItem item; const wchar_t* label; };
constexpr MenuMetric kMenuMetrics[] = {
    {DisplayItem::DownloadSpeed, L"Download speed"},
    {DisplayItem::UploadSpeed, L"Upload speed"},
    {DisplayItem::CpuUsage, L"CPU usage"},
    {DisplayItem::CpuTemp, L"CPU temperature"},
    {DisplayItem::RamUsage, L"RAM usage"},
    {DisplayItem::GpuUsage, L"GPU usage"},
    {DisplayItem::GpuTemp, L"GPU temperature"},
    {DisplayItem::CpuFrequency, L"CPU frequency"},
};
constexpr int kMetricRows = static_cast<int>(std::size(kMenuMetrics));
constexpr int kSettingsRow = kMetricRows + 1;
constexpr int kExitRow = kSettingsRow + 1;
constexpr int kMenuRows = kExitRow + 1;
}
TrayIcon::~TrayIcon() {
    if(menu_window_)DestroyWindow(menu_window_);
    Remove();
}

bool TrayIcon::Init(HWND hwnd_parent, HICON icon) {
    if (installed_) return true;

    ZeroMemory(&nid_, sizeof(nid_));
    nid_.cbSize           = sizeof(NOTIFYICONDATAW);
    nid_.hWnd             = hwnd_parent;
    nid_.uID              = 1;
    nid_.uFlags           = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
    nid_.uCallbackMessage = WM_TMP_TRAYICON;
    nid_.hIcon            = icon;
    wcscpy_s(nid_.szTip, L"TrafficMonitorPlus");

    // Use Shell_NotifyIconW version 4 for modern behaviour
    nid_.uVersion = NOTIFYICON_VERSION_4;

    if (!Shell_NotifyIconW(NIM_ADD, &nid_)) {
        return false;
    }
    Shell_NotifyIconW(NIM_SETVERSION, &nid_);
    installed_ = true;
    return true;
}

void TrayIcon::UpdateTooltip(const std::wstring& tip) {
    if (!installed_) return;
    // szTip is 128 wchars including null
    wcsncpy_s(nid_.szTip, tip.c_str(), _TRUNCATE);
    nid_.uFlags = NIF_TIP | NIF_SHOWTIP;
    Shell_NotifyIconW(NIM_MODIFY, &nid_);
}

void TrayIcon::ShowNotification(const std::wstring& title, const std::wstring& text) {
    if (!installed_) return;
    nid_.uFlags = NIF_INFO;
    wcsncpy_s(nid_.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(nid_.szInfo, text.c_str(), _TRUNCATE);
    nid_.dwInfoFlags = NIIF_WARNING;
    nid_.uTimeout = 5000;
    Shell_NotifyIconW(NIM_MODIFY, &nid_);
    nid_.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
}

void TrayIcon::Restore() {
    nid_.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
    installed_ = Shell_NotifyIconW(NIM_ADD, &nid_) != FALSE;
    nid_.uVersion = NOTIFYICON_VERSION_4;
    if (installed_) Shell_NotifyIconW(NIM_SETVERSION, &nid_);
}

void TrayIcon::Remove() {
    if (!installed_) return;
    Shell_NotifyIconW(NIM_DELETE, &nid_);
    installed_ = false;
}

// ────────────────────────────────────────────────────────
// Context Menu
// ────────────────────────────────────────────────────────
HMENU TrayIcon::CreateContextMenu(const std::map<DisplayItem,DisplayItemConfig>& items) {
    HMENU menu=CreatePopupMenu();
    if(!menu)return nullptr;
    AppendMenuW(menu,MF_STRING,IDM_SETTINGS,L"Settings...");
    const auto count=std::count_if(items.begin(),items.end(),[](const auto& entry){return entry.second.enabled;});
    for(const auto& metric:kMenuMetrics){
        const auto found=items.find(metric.item);const bool enabled=found!=items.end()&&found->second.enabled;
        AppendMenuW(menu,MF_STRING|(enabled?MF_CHECKED:MF_UNCHECKED)|(enabled&&count==1?MF_GRAYED:0),IDM_DISPLAY_ITEM_BASE+static_cast<UINT>(metric.item),metric.label);
    }
    AppendMenuW(menu,MF_SEPARATOR,0,nullptr);
    AppendMenuW(menu,MF_STRING,IDM_EXIT,L"Exit...");
    return menu;
}
void TrayIcon::ShowContextMenu(HWND hwnd, const std::map<DisplayItem,DisplayItemConfig>& items) {
    menu_items_=&items;
    selected_row_=-1;
    if(!menu_window_){
        WNDCLASSEXW wc{sizeof(wc)};wc.lpfnWndProc=MenuProc;wc.hInstance=GetModuleHandleW(nullptr);
        wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.lpszClassName=L"YashMonitorDisplayMenu";
        RegisterClassExW(&wc);
        menu_window_=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,wc.lpszClassName,L"Display",
            WS_POPUP|WS_BORDER,0,0,260,400,hwnd,nullptr,wc.hInstance,this);
    }
    if(!menu_window_)return;
    POINT point{};GetCursorPos(&point);MONITORINFO monitor{sizeof(monitor)};
    GetMonitorInfoW(MonitorFromPoint(point,MONITOR_DEFAULTTONEAREST),&monitor);
    const UINT dpi=GetDpiForWindow(hwnd);menu_row_height_=MulDiv(27,dpi?dpi:96,96);
    const int width=MulDiv(260,dpi?dpi:96,96),height=menu_row_height_*kMenuRows+8;
    int x=std::clamp(static_cast<int>(point.x),static_cast<int>(monitor.rcWork.left),(std::max)(static_cast<int>(monitor.rcWork.left),static_cast<int>(monitor.rcWork.right)-width));
    int y=std::clamp(static_cast<int>(point.y)-height,static_cast<int>(monitor.rcWork.top),(std::max)(static_cast<int>(monitor.rcWork.top),static_cast<int>(monitor.rcWork.bottom)-height));
    SetWindowPos(menu_window_,HWND_TOPMOST,x,y,width,height,SWP_SHOWWINDOW);
    SetForegroundWindow(menu_window_);SetFocus(menu_window_);InvalidateRect(menu_window_,nullptr,TRUE);
}
void TrayIcon::ActivateRow(HWND hwnd, int row) {
    UINT command = 0;
    if (row >= 1 && row <= kMetricRows) {
        int enabled_count = 0;
        bool is_currently_enabled = false;
        if (menu_items_) {
            for (const auto& [item, cfg] : *menu_items_) {
                if (cfg.enabled) ++enabled_count;
            }
            auto it = menu_items_->find(kMenuMetrics[row - 1].item);
            if (it != menu_items_->end() && it->second.enabled) {
                is_currently_enabled = true;
            }
        }
        if (is_currently_enabled && enabled_count <= 1) {
            return; // Protect the last enabled metric
        }
        command = IDM_DISPLAY_ITEM_BASE + static_cast<UINT>(kMenuMetrics[row - 1].item);
    } else if (row == kSettingsRow) {
        command = IDM_SETTINGS;
        ShowWindow(hwnd, SW_HIDE);
        if (on_menu_dismissed) on_menu_dismissed();
    } else if (row == kExitRow) {
        command = IDM_EXIT;
        bool ok = confirm_exit ? confirm_exit() : (MessageBoxW(hwnd, L"Close Yash Monitor?", L"Yash Monitor", MB_YESNO | MB_DEFBUTTON2) == IDYES);
        if (!ok) return;
        ShowWindow(hwnd, SW_HIDE);
        if (on_menu_dismissed) on_menu_dismissed();
    }
    if (command) {
        SendMessageW(nid_.hWnd, WM_COMMAND, command, 0);
    }
    if (IsWindow(hwnd)) InvalidateRect(hwnd, nullptr, TRUE);
}
LRESULT CALLBACK TrayIcon::MenuProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    auto self=reinterpret_cast<TrayIcon*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE){self=static_cast<TrayIcon*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
    if(!self)return DefWindowProcW(hwnd,msg,wp,lp);
    if(msg==WM_ACTIVATE&&LOWORD(wp)==WA_INACTIVE){ShowWindow(hwnd,SW_HIDE);if(self->on_menu_dismissed)self->on_menu_dismissed();return 0;}
    if(msg==WM_KEYDOWN){
        if(wp==VK_ESCAPE){ShowWindow(hwnd,SW_HIDE);if(self->on_menu_dismissed)self->on_menu_dismissed();return 0;}
        if(wp==VK_DOWN){
            self->selected_row_=(self->selected_row_<1||self->selected_row_>=kExitRow)?1:(self->selected_row_+1);
            InvalidateRect(hwnd,nullptr,TRUE);
            return 0;
        }
        if(wp==VK_UP){
            self->selected_row_=(self->selected_row_<=1||self->selected_row_>kExitRow)?kExitRow:(self->selected_row_-1);
            InvalidateRect(hwnd,nullptr,TRUE);
            return 0;
        }
        if(wp==VK_HOME){
            self->selected_row_=1;
            InvalidateRect(hwnd,nullptr,TRUE);
            return 0;
        }
        if(wp==VK_END){
            self->selected_row_=kExitRow;
            InvalidateRect(hwnd,nullptr,TRUE);
            return 0;
        }
        if(wp==VK_RETURN||wp==VK_SPACE){
            if(self->selected_row_>=1&&self->selected_row_<=kExitRow){
                self->ActivateRow(hwnd,self->selected_row_);
            }
            return 0;
        }
    }
    if(msg==WM_MOUSEMOVE){
        const int row=(GET_Y_LPARAM(lp)-4)/self->menu_row_height_;
        if(row>=1&&row<=kExitRow&&row!=self->selected_row_){
            self->selected_row_=row;
            InvalidateRect(hwnd,nullptr,TRUE);
        }
        return 0;
    }
    if(msg==WM_PAINT){
        PAINTSTRUCT paint{};HDC dc=BeginPaint(hwnd,&paint);RECT bounds{};GetClientRect(hwnd,&bounds);
        FillRect(dc,&bounds,GetSysColorBrush(COLOR_MENU));SetBkMode(dc,TRANSPARENT);
        HFONT font=CreateFontW(-MulDiv(13,GetDpiForWindow(hwnd),96),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,0,0,L"Segoe UI");
        auto old=SelectObject(dc,font);
        int enabled_count=0;
        if(self->menu_items_){
            for(const auto& [item,cfg]:*self->menu_items_)if(cfg.enabled)++enabled_count;
        }

        for(int row=0;row<kMenuRows;++row){
            RECT r{4,row*self->menu_row_height_+4,bounds.right-4,(row+1)*self->menu_row_height_+4};
            const bool is_selected=(row==self->selected_row_);
            bool is_disabled=false;
            if(row>=1&&row<=kMetricRows&&self->menu_items_){
                auto item=self->menu_items_->find(kMenuMetrics[row-1].item);
                if(item!=self->menu_items_->end()&&item->second.enabled&&enabled_count<=1)is_disabled=true;
            }
            if(is_selected){
                FillRect(dc,&r,GetSysColorBrush(COLOR_HIGHLIGHT));
                SetTextColor(dc,GetSysColor(COLOR_HIGHLIGHTTEXT));
            }else{
                SetTextColor(dc,is_disabled?GetSysColor(COLOR_GRAYTEXT):(row==0?GetSysColor(COLOR_GRAYTEXT):GetSysColor(COLOR_MENUTEXT)));
            }
            RECT text_r{32,r.top,r.right-4,r.bottom};
            const wchar_t* label=row==0?L"DISPLAY":row<=kMetricRows?kMenuMetrics[row-1].label:row==kSettingsRow?L"Settings...":L"Exit...";
            DrawTextW(dc,label,-1,&text_r,DT_SINGLELINE|DT_VCENTER);
            if(row>=1&&row<=kMetricRows&&self->menu_items_){
                auto item=self->menu_items_->find(kMenuMetrics[row-1].item);
                if(item!=self->menu_items_->end()&&item->second.enabled){
                    RECT check{10,r.top,30,r.bottom};
                    DrawTextW(dc,L"\u2713",-1,&check,DT_SINGLELINE|DT_VCENTER);
                }
            }
        }
        SelectObject(dc,old);DeleteObject(font);EndPaint(hwnd,&paint);return 0;
    }
    if(msg==WM_LBUTTONUP){
        const int row=(GET_Y_LPARAM(lp)-4)/self->menu_row_height_;
        if(row>=1&&row<=kExitRow){
            self->selected_row_=row;
            self->ActivateRow(hwnd,row);
        }
        return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}
} // namespace tmp
