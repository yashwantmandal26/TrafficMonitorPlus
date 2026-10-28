
// TrafficMonitorDlg.cpp: Implementation file
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "TrafficMonitorDlg.h"
#include "afxdialogex.h"
#include "Test.h"
#include "PluginManagerDlg.h"
#include "SetItemOrderDlg.h"
#include "WindowsSettingHelper.h"
#include "PluginInfoDlg.h"
#include "WIC.h"
#include "SupportedRenderEnums.h"
#include "ClassicalTaskbarDlg.h"
#include "Win11TaskbarDlg.h"
#include "WineTaskbarDlg.h"
#include "TaskbarHelper.h"
#include "SkinManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



// CTrafficMonitorDlg dialog box

//Static member initialization
unsigned int CTrafficMonitorDlg::m_WM_TASKBARCREATED{ ::RegisterWindowMessage(_T("TaskbarCreated")) };  //Register messages created by the taskbar

CTrafficMonitorDlg::CTrafficMonitorDlg(CWnd* pParent /*=NULL*/)
    : CDialog(IDD_TRAFFICMONITOR_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_desktop_dc = ::GetDC(NULL);
}

CTrafficMonitorDlg::~CTrafficMonitorDlg()
{
    free(m_pIfTable);

    if (m_tBarDlg != nullptr)
    {
        delete m_tBarDlg;
        m_tBarDlg = nullptr;
    }

    ::ReleaseDC(NULL, m_desktop_dc);

    // Destroy notify area icon handles loaded via LoadImage in OnInitDialog
    // LoadImage creates a copy of the icon resource; each must be freed with DestroyIcon
    for (int i = 0; i < MAX_NOTIFY_ICON; i++)
    {
        if (theApp.m_notify_icons[i] != NULL)
        {
            ::DestroyIcon(theApp.m_notify_icons[i]);
            theApp.m_notify_icons[i] = NULL;
        }
    }
}

CTaskBarDlg* CTrafficMonitorDlg::GetTaskbarWindow() const
{
    if (IsTaskbarWndValid())
        return m_tBarDlg;
    else
        return nullptr;
}

CTrafficMonitorDlg* CTrafficMonitorDlg::Instance()
{
    return dynamic_cast<CTrafficMonitorDlg*>(theApp.m_pMainWnd);
}

void CTrafficMonitorDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTrafficMonitorDlg, CDialog)
    ON_WM_QUERYDRAGICON()
    ON_WM_TIMER()
    ON_WM_RBUTTONUP()
    ON_WM_LBUTTONDOWN()
    ON_COMMAND(ID_NETWORK_INFO, &CTrafficMonitorDlg::OnNetworkInfo)
    ON_COMMAND(ID_ALWAYS_ON_TOP, &CTrafficMonitorDlg::OnAlwaysOnTop)
    ON_WM_INITMENUPOPUP()
    ON_COMMAND(ID_TRANSPARENCY_100, &CTrafficMonitorDlg::OnTransparency100)
    ON_COMMAND(ID_TRANSPARENCY_80, &CTrafficMonitorDlg::OnTransparency80)
    ON_COMMAND(ID_TRANSPARENCY_60, &CTrafficMonitorDlg::OnTransparency60)
    ON_COMMAND(ID_TRANSPARENCY_40, &CTrafficMonitorDlg::OnTransparency40)
    ON_WM_CLOSE()
    ON_WM_INITMENU()
    ON_COMMAND(ID_LOCK_WINDOW_POS, &CTrafficMonitorDlg::OnLockWindowPos)
    ON_WM_MOVE()
    ON_MESSAGE(MY_WM_NOTIFYICON, &CTrafficMonitorDlg::OnNotifyIcon)
    ON_COMMAND(ID_SHOW_NOTIFY_ICON, &CTrafficMonitorDlg::OnShowNotifyIcon)
    ON_WM_DESTROY()
    ON_COMMAND(ID_SHOW_CPU_MEMORY, &CTrafficMonitorDlg::OnShowCpuMemory)
    ON_COMMAND(ID_MOUSE_PENETRATE, &CTrafficMonitorDlg::OnMousePenetrate)
    ON_COMMAND(ID_SHOW_TASK_BAR_WND, &CTrafficMonitorDlg::OnShowTaskBarWnd)
    ON_COMMAND(ID_APP_ABOUT, &CTrafficMonitorDlg::OnAppAbout)
    ON_COMMAND(ID_SHOW_CPU_MEMORY2, &CTrafficMonitorDlg::OnShowCpuMemory2)
    ON_COMMAND(ID_SHOW_MAIN_WND, &CTrafficMonitorDlg::OnShowMainWnd)
    ON_COMMAND(ID_CHANGE_SKIN, &CTrafficMonitorDlg::OnChangeSkin)
    ON_REGISTERED_MESSAGE(m_WM_TASKBARCREATED, &CTrafficMonitorDlg::OnTaskBarCreated)
    ON_COMMAND(ID_TRAFFIC_HISTORY, &CTrafficMonitorDlg::OnTrafficHistory)
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDBLCLK()
    ON_COMMAND(ID_OPTIONS, &CTrafficMonitorDlg::OnOptions)
    ON_COMMAND(ID_OPTIONS2, &CTrafficMonitorDlg::OnOptions2)
    ON_MESSAGE(WM_EXITMENULOOP, &CTrafficMonitorDlg::OnExitmenuloop)
    ON_COMMAND(ID_CHANGE_NOTIFY_ICON, &CTrafficMonitorDlg::OnChangeNotifyIcon)
    ON_COMMAND(ID_ALOW_OUT_OF_BORDER, &CTrafficMonitorDlg::OnAlowOutOfBorder)
    ON_COMMAND(ID_CHECK_UPDATE, &CTrafficMonitorDlg::OnCheckUpdate)
    ON_MESSAGE(WM_TASKBAR_MENU_POPED_UP, &CTrafficMonitorDlg::OnTaskbarMenuPopedUp)
    ON_COMMAND(ID_SHOW_NET_SPEED, &CTrafficMonitorDlg::OnShowNetSpeed)
    ON_WM_QUERYENDSESSION()
    ON_WM_PAINT()
    ON_MESSAGE(WM_DPICHANGED, &CTrafficMonitorDlg::OnDpichanged)
    ON_MESSAGE(WM_TASKBAR_WND_CLOSED, &CTrafficMonitorDlg::OnTaskbarWndClosed)
    ON_MESSAGE(WM_MONITOR_INFO_UPDATED, &CTrafficMonitorDlg::OnMonitorInfoUpdated)
    ON_MESSAGE(WM_DISPLAYCHANGE, &CTrafficMonitorDlg::OnDisplaychange)
    ON_WM_EXITSIZEMOVE()
    ON_COMMAND(ID_PLUGIN_MANAGE, &CTrafficMonitorDlg::OnPluginManage)
    ON_MESSAGE(WM_REOPEN_TASKBAR_WND, &CTrafficMonitorDlg::OnReopenTaksbarWnd)
    ON_COMMAND(ID_OPEN_TASK_MANAGER, &CTrafficMonitorDlg::OnOpenTaskManager)
    ON_MESSAGE(WM_SETTINGS_APPLIED, &CTrafficMonitorDlg::OnSettingsApplied)
    ON_COMMAND(ID_DISPLAY_SETTINGS, &CTrafficMonitorDlg::OnDisplaySettings)
    ON_WM_LBUTTONUP()
    ON_COMMAND(ID_REFRESH_CONNECTION_LIST, &CTrafficMonitorDlg::OnRefreshConnectionList)
    ON_MESSAGE(WM_TABLET_QUERYSYSTEMGESTURESTATUS, &CTrafficMonitorDlg::OnTabletQuerysystemgesturestatus)
    ON_COMMAND(ID_PLUGIN_OPTIONS, &CTrafficMonitorDlg::OnPluginOptions)
    ON_COMMAND(ID_PLUGIN_DETAIL, &CTrafficMonitorDlg::OnPluginDetail)
    ON_COMMAND(ID_PLUGIN_OPTIONS_TASKBAR, &CTrafficMonitorDlg::OnPluginOptionsTaksbar)
    ON_COMMAND(ID_PLUGIN_DETAIL_TASKBAR, &CTrafficMonitorDlg::OnPluginDetailTaksbar)
    ON_WM_POWERBROADCAST()
    ON_WM_DWMCOLORIZATIONCOLORCHANGED()
END_MESSAGE_MAP()


CString CTrafficMonitorDlg::GetMouseTipsInfo()
{
    CString tip_info;
    CString temp;
    temp.Format(_T("%s: %s\r\n (%s: %s/%s: %s)"), CCommon::LoadText(IDS_TRAFFIC_USED_TODAY),
        CCommon::KBytesToString((theApp.m_today_up_traffic + theApp.m_today_down_traffic) / 1024u),
        CCommon::LoadText(IDS_UPLOAD), CCommon::KBytesToString(theApp.m_today_up_traffic / 1024u),
        CCommon::LoadText(IDS_DOWNLOAD), CCommon::KBytesToString(theApp.m_today_down_traffic / 1024u)
    );
    tip_info += temp;
    const CSkinFile::Layout& skin_layout{ theApp.m_cfg_data.m_show_more_info ? m_skin.GetLayoutInfo().layout_l : m_skin.GetLayoutInfo().layout_s }; //Current skin layout
    if (!skin_layout.GetItem(TDI_UP).show)      //If the upload speed is not displayed in the main window, the upload speed will be displayed in the prompt message.
    {
        temp.Format(_T("\r\n%s: %s/s"), CCommon::LoadText(IDS_UPLOAD),
            CCommon::DataSizeToString(theApp.m_out_speed, theApp.m_main_wnd_data));
        tip_info += temp;
    }
    if (!skin_layout.GetItem(TDI_DOWN).show)
    {
        temp.Format(_T("\r\n%s: %s/s"), CCommon::LoadText(IDS_DOWNLOAD),
            CCommon::DataSizeToString(theApp.m_in_speed, theApp.m_main_wnd_data));
        tip_info += temp;
    }
    if (!skin_layout.GetItem(TDI_CPU).show)
    {
        temp.Format(_T("\r\n%s: %d %%"), CCommon::LoadText(IDS_CPU_USAGE), theApp.m_cpu_usage);
        tip_info += temp;
    }
    if (!skin_layout.GetItem(TDI_MEMORY).show)
    {
        temp.Format(_T("\r\n%s: %s/%s (%d %%)"), CCommon::LoadText(IDS_MEMORY_USAGE),
            CCommon::KBytesToString(theApp.m_used_memory),
            CCommon::KBytesToString(theApp.m_total_memory), theApp.m_memory_usage);
        tip_info += temp;
    }
    else
    {
        temp.Format(_T("\r\n%s: %s/%s"), CCommon::LoadText(IDS_MEMORY_USAGE),
            CCommon::KBytesToString(theApp.m_used_memory),
            CCommon::KBytesToString(theApp.m_total_memory));
        tip_info += temp;
    }
    if (!skin_layout.GetItem(TDI_CPU_FREQ).show && theApp.m_cpu_freq >= 0)
    {
        temp.Format(_T("\r\n%s: %s"), CCommon::LoadText(IDS_CPU_FREQ), CCommon::FreqToString(theApp.m_cpu_freq, theApp.m_main_wnd_data));
        tip_info += temp;
    }
    if (!skin_layout.GetItem(TDI_GPU_USAGE).show && theApp.m_gpu_usage >= 0)
    {
        temp.Format(_T("\r\n%s: %d %%"), CCommon::LoadText(IDS_GPU_USAGE), theApp.m_gpu_usage);
        tip_info += temp;
    }
#ifndef WITHOUT_TEMPERATURE
    if (IsTemperatureNeeded())
    {
        if (theApp.m_general_data.IsHardwareEnable(HI_CPU) && !skin_layout.GetItem(TDI_CPU_TEMP).show && theApp.m_cpu_temperature > 0)
        {
            temp.Format(_T("\r\n%s: %s"), CCommon::LoadText(IDS_CPU_TEMPERATURE), CCommon::TemperatureToString(theApp.m_cpu_temperature, theApp.m_main_wnd_data));
            tip_info += temp;
        }
        if (theApp.m_general_data.IsHardwareEnable(HI_GPU) && !skin_layout.GetItem(TDI_GPU_TEMP).show && theApp.m_gpu_temperature > 0)
        {
            temp.Format(_T("\r\n%s: %s"), CCommon::LoadText(IDS_GPU_TEMPERATURE), CCommon::TemperatureToString(theApp.m_gpu_temperature, theApp.m_main_wnd_data));
            tip_info += temp;
        }
        if (theApp.m_general_data.IsHardwareEnable(HI_HDD) && !skin_layout.GetItem(TDI_HDD_TEMP).show && theApp.m_hdd_temperature > 0)
        {
            temp.Format(_T("\r\n%s: %s"), CCommon::LoadText(IDS_HDD_TEMPERATURE), CCommon::TemperatureToString(theApp.m_hdd_temperature, theApp.m_main_wnd_data));
            tip_info += temp;
        }
        if (theApp.m_general_data.IsHardwareEnable(HI_MBD) && !skin_layout.GetItem(TDI_MAIN_BOARD_TEMP).show && theApp.m_main_board_temperature > 0)
        {
            temp.Format(_T("\r\n%s: %s"), CCommon::LoadText(IDS_MAINBOARD_TEMPERATURE), CCommon::TemperatureToString(theApp.m_main_board_temperature, theApp.m_main_wnd_data));
            tip_info += temp;
        }
    }
#endif
    if (!skin_layout.GetItem(TDI_HDD_USAGE).show && theApp.m_hdd_usage >= 0)
    {
        temp.Format(_T("\r\n%s: %d %%"), CCommon::LoadText(IDS_HDD_USAGE), theApp.m_hdd_usage);
        tip_info += temp;
}

    //Add mouse hints for plugin projects
    tip_info += theApp.GetPlauginTooltipInfo().c_str();

    //if (IsTaskbarWndValid())
    //{
    //    tip_info += L"\r\n";
    //    tip_info += m_tBarDlg->GetTaskbarDebugString().c_str();
    //}

    if (theApp.m_taskbar_data.show_refresh_rate)
    {
        DEVMODE dm{};
        dm.dmSize = sizeof(dm);
        if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dm) && dm.dmDisplayFrequency > 0)
        {
            temp.Format(_T("\r\n%s: %d Hz"), _T("Refresh Rate"), (int)dm.dmDisplayFrequency);
            tip_info += temp;
        }
    }
    return tip_info;
}

void CTrafficMonitorDlg::SetTransparency()
{
    SetTransparency(theApp.m_cfg_data.m_transparency);
}

void CTrafficMonitorDlg::SetTransparency(int transparency)
{
    SetWindowLong(m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    if (m_skin.IsPNG())
    {
        m_skin.SetAlpha(transparency * 255 / 100);
    }
    else
    {
        SetLayeredWindowAttributes(0, transparency * 255 / 100, LWA_ALPHA);  //Transparency value range is 0~255
    }
}

void CTrafficMonitorDlg::SetAlwaysOnTop()
{
    //if (!m_is_foreground_fullscreen || (m_is_foreground_fullscreen && !theApp.m_main_wnd_data.hide_main_wnd_when_fullscreen))
    //{
    //  if (m_always_on_top)
    //      SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); //Set top
    //  else
    //      SetWindowPos(&wndNoTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); //Cancel top
    //}
    if (theApp.m_cfg_data.m_hide_main_window)
        return;
    else if (theApp.m_main_wnd_data.hide_main_wnd_when_fullscreen && m_is_foreground_fullscreen)        //When a program is set to full screen and the floating window is hidden and a program is running in full screen, the top operation will not be performed.
        return;

    if (theApp.m_main_wnd_data.m_always_on_top)
        SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); //Set top
    else
        SetWindowPos(&wndNoTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); //Cancel top

}

void CTrafficMonitorDlg::SetMousePenetrate()
{
    if (theApp.m_main_wnd_data.m_mouse_penetrate)
    {
        SetWindowLong(m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);     //Set mouse penetration
    }
    else
    {
        SetWindowLong(m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE) & (~WS_EX_TRANSPARENT));      //Cancel mouse penetration
    }
}

POINT CTrafficMonitorDlg::CalculateWindowMoveOffset(CRect rect, bool screen_changed)
{
    POINT mov{};    // required offset
    if (m_screen_rects.size() != 0)
    {
        // Make sure the window is completely within one monitor and visible, determine the movement distance and move in the direction with the smaller required movement distance
        LONG mov_xy = 0;          // Record moving distance
        int i = 0;
        for (auto& a : m_screen_rects)
        {
            LONG x = 0, y = 0;
            if (rect.left < a.left)                 // Need to move right
                x = a.left - rect.left;
            if (rect.top < a.top)                   // need to move down
                y = a.top - rect.top;

            //Prevent the number of m_last_screen_rects from being less than the number of m_screen_rects when connecting to a new screen, resulting in access to an overly large index.
            if (i >= m_last_screen_rects.size())
            {
                break;
            }
            CRect last_screen_rect = m_last_screen_rects[i];
            if (screen_changed && a != last_screen_rect)
            {
                float proportion_width = (float)rect.left / (last_screen_rect.right - rect.Width());
                float proportion_height = (float)rect.top / (last_screen_rect.bottom - rect.Height());
                x = (a.right - rect.Width()) * proportion_width - rect.left;
                y = (a.bottom - rect.Height()) * proportion_height - rect.top;
            }
            else
            {
                if (rect.right > a.right)          // Need to move left
                    x = a.right - rect.right;
                if (rect.bottom > a.bottom)        // need to move up
                    y = a.bottom - rect.bottom;
            }
            if (x == 0 && y == 0)           // The window is already within a monitor
            {
                mov.x = 0;
                mov.y = 0;
                break;
            }
            else if (abs(x) + abs(y) < mov_xy || mov_xy == 0)
            {
                mov.x = x;
                mov.y = y;
                mov_xy = abs(x) + abs(y);
            }
            i++;
        }
    }
    return mov;
}

void CTrafficMonitorDlg::CheckWindowPos(bool screen_changed)
{
    if (!theApp.m_main_wnd_data.m_alow_out_of_border)
    {
        CRect rect;
        GetWindowRect(rect);
        MoveWindow(rect + CalculateWindowMoveOffset(rect, screen_changed));
    }
}

void CTrafficMonitorDlg::GetScreenSize()
{
    m_screen_size.cx = GetSystemMetrics(SM_CXSCREEN);
    m_screen_size.cy = GetSystemMetrics(SM_CYSCREEN);

    //::SystemParametersInfo(SPI_GETWORKAREA, 0, &m_screen_rect, 0); // Get the work area size

    //Get the size of all screen workspaces
    m_last_screen_rects = m_screen_rects;
    m_screen_rects.clear();
    Monitors monitors;
    for (auto& a : monitors.monitorinfos)
    {
        m_screen_rects.push_back(a.rcWork);
    }
}


void CTrafficMonitorDlg::AutoSelect()
{
    unsigned __int64 max_in_out_bytes{};
    unsigned __int64 in_out_bytes;
    //m_connection_selected = m_connections[0].index;
    m_connection_selected = 0;
    //When automatically selecting a connection, find the connection with the largest sum of sent and received bytes and set it as the currently viewed connection
    for (size_t i{}; i < m_connections.size(); i++)
    {
        auto table = GetConnectIfTable(i);
        if (table.dwOperStatus == IF_OPER_STATUS_OPERATIONAL)     //Only select connections with normal network status
        {
            in_out_bytes = table.dwInOctets + table.dwOutOctets;
            if (in_out_bytes > max_in_out_bytes)
            {
                max_in_out_bytes = in_out_bytes;
                m_connection_selected = i;
            }
        }
    }
    theApp.m_cfg_data.m_connection_name = GetConnection(m_connection_selected).description_2;
    m_connection_change_flag = true;
}

void CTrafficMonitorDlg::IniConnection()
{
    //Allocate the required size of memory for m_pIfTable
    free(m_pIfTable);
    m_dwSize = sizeof(MIB_IFTABLE);
    m_pIfTable = (MIB_IFTABLE*)malloc(m_dwSize);
    int rtn;
    rtn = GetIfTable(m_pIfTable, &m_dwSize, FALSE);
    if (rtn == ERROR_INSUFFICIENT_BUFFER)	//If the function return value is ERROR_INSUFFICIENT_BUFFER, it means that the size of m_pIfTable is not enough.
    {
        free(m_pIfTable);
        m_pIfTable = (MIB_IFTABLE*)malloc(m_dwSize);	//Re-open a piece of memory with a new size
    }
    GetIfTable(m_pIfTable, &m_dwSize, FALSE);

    //Get all current connections and save them in the m_connections container
    if (!theApp.m_general_data.show_all_interface)
    {
        m_connections.clear();
        vector<NetWorkConection> connections;
        CAdapterCommon::GetAdapterInfo(connections);
        for (const auto& item : connections)
        {
            if (!theApp.m_general_data.connections_hide.Contains(CCommon::StrToUnicode(item.description.c_str())))
                m_connections.push_back(item);
        }
        CAdapterCommon::GetIfTableInfo(m_connections, m_pIfTable);
    }
    else
    {
        CAdapterCommon::GetAllIfTableInfo(m_connections, m_pIfTable);
    }

    //If "Select All" is set when "Show All Network Connections" is set, it is changed to "Auto Select"
    if (theApp.m_general_data.show_all_interface && theApp.m_cfg_data.m_select_all)
    {
        theApp.m_cfg_data.m_select_all = false;
        theApp.m_cfg_data.m_auto_select = true;
    }

    //Write debug log
    if (theApp.m_debug_log)
    {
        CString log_str;
        log_str += _T("Initializing network connection...\n");
        log_str += _T("Connection list:\n");
        for (size_t i{}; i < m_connections.size(); i++)
        {
            log_str += m_connections[i].description.c_str();
            log_str += _T(", ");
            log_str += CCommon::IntToString(m_connections[i].index);
            log_str += _T("\n");
        }
        log_str += _T("IfTable:\n");
        for (size_t i{}; i < m_pIfTable->dwNumEntries; i++)
        {
            log_str += CCommon::IntToString(i);
            log_str += _T(" ");
            log_str += (const char*)m_pIfTable->table[i].bDescr;
            log_str += _T("\n");
        }
        CCommon::WriteLog(log_str, (theApp.m_config_dir + L".\\connections.log").c_str());
    }

    //if (m_connection_selected < 0 || m_connection_selected >= m_connections.size() || m_auto_select)
    //  AutoSelect();
    //Select network connection
    if (theApp.m_cfg_data.m_auto_select)    //automatic selection
    {
        if (m_restart_cnt != -1)    //When m_restart_cnt is not equal to -1, that is, it is not the first initialization, you need to delay for 5 seconds before reinitializing the connection.
        {
            KillTimer(DELAY_TIMER);
            SetTimer(DELAY_TIMER, 5000, NULL);
        }
        else
        {
            AutoSelect();
        }
    }
    else        //Find the network named last selected connection
    {
        m_connection_selected = 0;
        for (size_t i{}; i < m_connections.size(); i++)
        {
            if (m_connections[i].description_2 == m_connection_name_preferd)
                m_connection_selected = i;
        }
    }
    if (m_connection_selected < 0 || m_connection_selected >= m_connections.size())
        m_connection_selected = 0;
    theApp.m_cfg_data.m_connection_name = GetConnection(m_connection_selected).description_2;

    //Add corresponding items to the menu based on the obtained connection
    IniConnectionMenu(theApp.m_main_menu.GetSubMenu(0)->GetSubMenu(0));      //Add an item to the "Select a network connection" submenu item
    IniConnectionMenu(theApp.m_main_menu_plugin.GetSubMenu(0)->GetSubMenu(0));      //Add an item to the "Select a network connection" submenu item

    IniTaskBarConnectionMenu();     //Initialize the "Select Network Connection" submenu item in the taskbar window

    m_restart_cnt++;    //Record initialization times
    m_connection_change_flag = true;
}

MIB_IFROW CTrafficMonitorDlg::GetConnectIfTable(int connection_index)
{
    if (connection_index >= 0 && connection_index < static_cast<int>(m_connections.size()))
    {
        int index = m_connections[connection_index].index;
        if (m_pIfTable != nullptr && index >= 0 && index < m_pIfTable->dwNumEntries)
            return m_pIfTable->table[index];
    }
    return MIB_IFROW();
}

NetWorkConection CTrafficMonitorDlg::GetConnection(int connection_index)
{
    if (connection_index >= 0 && connection_index < static_cast<int>(m_connections.size()))
        return m_connections[connection_index];
    else
        return NetWorkConection();
}

void CTrafficMonitorDlg::IniConnectionMenu(CMenu* pMenu)
{
    if (pMenu == nullptr)
        return;
    {
        //First delete all menu items after ID_SELECT_ALL_CONNECTION
        int start_pos = CCommon::GetMenuItemPosition(pMenu, ID_SELECT_ALL_CONNECTION) + 1;
        while (pMenu->GetMenuItemCount() > start_pos)
        {
            pMenu->DeleteMenu(start_pos, MF_BYPOSITION);
        }

        CString connection_descr;
        for (size_t i{}; i < m_connections.size(); i++)
        {
            connection_descr = CCommon::StrToUnicode(m_connections[i].description.c_str()).c_str();
            pMenu->AppendMenu(MF_STRING | MF_ENABLED, ID_SELECT_ALL_CONNECTION + i + 1, connection_descr);
        }

        //Added "Refresh Network List" command
        pMenu->AppendMenu(MF_SEPARATOR);
        pMenu->AppendMenu(MF_STRING | MF_ENABLED, ID_REFRESH_CONNECTION_LIST, CCommon::LoadText(IDS_REFRESH_CONNECTION_LIST));
    }
}

void CTrafficMonitorDlg::IniTaskBarConnectionMenu()
{
    // Network selection in menu disabled for streamlined minimal menu
}

void CTrafficMonitorDlg::SetConnectionMenuState(CMenu* pMenu)
{
    if (pMenu == nullptr)
        return;
    if (theApp.m_cfg_data.m_select_all)
        pMenu->CheckMenuRadioItem(0, m_connections.size() + 1, 1, MF_BYPOSITION | MF_CHECKED);
    else if (theApp.m_cfg_data.m_auto_select)       //When m_auto_select is true, it is automatically selected and the first item of the menu is selected.
        pMenu->CheckMenuRadioItem(0, m_connections.size() + 1, 0, MF_BYPOSITION | MF_CHECKED);
    else        //When m_auto_select is false, it is not automatically selected. The corresponding item is selected based on the value of m_connection_selected.
        pMenu->CheckMenuRadioItem(0, m_connections.size() + 1, m_connection_selected + 2, MF_BYPOSITION | MF_CHECKED);

    //When not set to "Select All", the current selection is set as the default menu item (displayed in bold)
    if (!theApp.m_cfg_data.m_select_all)
        pMenu->SetDefaultItem(m_connection_selected + 2, TRUE);
    else
        pMenu->SetDefaultItem(-1, TRUE);
}

void CTrafficMonitorDlg::CloseTaskBarWnd()
{
    if (m_tBarDlg != nullptr)
    {
        if (IsTaskbarWndValid())
            m_tBarDlg->OnCancel();
        delete m_tBarDlg;
        m_tBarDlg = nullptr;
        theApp.m_taskbar_data.update_layered_window_error_code = 0;
    }
}

void CTrafficMonitorDlg::OpenTaskBarWnd()
{
    // Force initialization of theApp.m_is_windows11_taskbar value
    theApp.CheckWindows11Taskbar();
    if (theApp.m_win_version.IsWine())
        m_tBarDlg = new CWineTaskbarDlg();
    else if (theApp.IsWindows11Taskbar())
        m_tBarDlg = new CWin11TaskbarDlg();
    else
        m_tBarDlg = new CClassicalTaskbarDlg();

    CSupportedRenderEnums supported_render_enums{};
    CTaskBarDlg::DisableRenderFeatureIfNecessary(supported_render_enums);
    auto render_type = supported_render_enums.GetAutoFitEnum();
    // WS_EX_LAYERED and WS_EX_NOREDIRECTIONBITMAP can coexist, see Microsoft sample code
    // https://github.com/microsoft/Windows-classic-samples/blob/7cbd99ac1d2b4a0beffbaba29ea63d024ceff700/Samples/DynamicDPI/cpp/SampleDesktopWindow.cpp#L179
    // But WS_EX_NOREDIRECTIONBITMAP seems to cause UpdateLayeredWindowIndirect to fail
    switch (render_type)
    {
        using namespace DrawCommonHelper;
    case RenderType::D2D1_WITH_DCOMPOSITION:
        m_tBarDlg->Create(IDD_TASK_BAR_DIALOG_NOREDIRECTIONBITMAP, this);
        break;
    // Other values including RenderType::D2D1
    default:
        m_tBarDlg->Create(IDD_TASK_BAR_DIALOG, this);
        break;
    }
    m_tBarDlg->ShowWindow(SW_SHOW);
    //m_tBarDlg->ShowInfo();
    //IniTaskBarConnectionMenu();
}

void CTrafficMonitorDlg::AddNotifyIcon()
{
    if (theApp.m_cfg_data.m_show_task_bar_wnd)
        CloseTaskBarWnd();
    //Add notification bar icon
    ::Shell_NotifyIcon(NIM_ADD, &m_ntIcon);
    if (theApp.m_cfg_data.m_show_task_bar_wnd)
        OpenTaskBarWnd();
}

void CTrafficMonitorDlg::DeleteNotifyIcon()
{
    if (theApp.m_cfg_data.m_show_task_bar_wnd)
        CloseTaskBarWnd();
    //Delete notification bar icon
    ::Shell_NotifyIcon(NIM_DELETE, &m_ntIcon);
    if (theApp.m_cfg_data.m_show_task_bar_wnd)
        OpenTaskBarWnd();
}

void CTrafficMonitorDlg::ShowNotifyTip(const wchar_t* title, const wchar_t* message)
{
    //To display the notification area prompt, you must first display the notification area icon.
    if (!theApp.m_general_data.show_notify_icon)
    {
        //Add notification bar icon
        AddNotifyIcon();
    }
    //Show notification prompts
    m_ntIcon.uFlags |= NIF_INFO;
    //wcscpy_s(m_ntIcon.szInfo, message ? message : _T(""));
    //wcscpy_s(m_ntIcon.szInfoTitle, title ? title : _T(""));
    CCommon::WStringCopy(m_ntIcon.szInfo, 256, message);
    CCommon::WStringCopy(m_ntIcon.szInfoTitle, 64, title);
    ::Shell_NotifyIcon(NIM_MODIFY, &m_ntIcon);
    m_ntIcon.uFlags &= ~NIF_INFO;

    //If the notification area icon is not displayed, delete the notification area icon after a period of time after the notification pops up.
    if (!theApp.m_general_data.show_notify_icon)
    {
        //Delete the notification area icon after a certain delay
        KillTimer(DELETE_NOTIFY_ICON_TIMER);
        SetTimer(DELETE_NOTIFY_ICON_TIMER, 8000, NULL);
    }
}

void CTrafficMonitorDlg::UpdateNotifyIconTip()
{
    CString strTip;         //Tips displayed when the mouse points to the icon
#ifdef _DEBUG
    strTip = CCommon::LoadText(IDS_TRAFFICMONITOR, _T(" (Debug)"));
#else
    strTip = CCommon::LoadText(IDS_TRAFFICMONITOR);
#endif

    CString in_speed = CCommon::DataSizeToString(theApp.m_in_speed);
    CString out_speed = CCommon::DataSizeToString(theApp.m_out_speed);

    strTip += CCommon::StringFormat(_T("\r\n<%1%>: <%2%>/s"), { CCommon::LoadText(IDS_UPLOAD), out_speed });
    strTip += CCommon::StringFormat(_T("\r\n<%1%>: <%2%>/s"), { CCommon::LoadText(IDS_DOWNLOAD), in_speed });
    strTip += CCommon::StringFormat(_T("\r\nCPU: <%1%> %"), { theApp.m_cpu_usage });
    strTip += CCommon::StringFormat(_T("\r\n<%1%>: <%2%> %"), { CCommon::LoadText(IDS_MEMORY), theApp.m_memory_usage });
    if (IsTemperatureNeeded())
    {
        if (theApp.m_general_data.IsHardwareEnable(HI_GPU) && theApp.m_gpu_usage >= 0)
            strTip += CCommon::StringFormat(_T("\r\n<%1%>: <%2%> %"), { CCommon::LoadText(IDS_GPU_USAGE), theApp.m_gpu_usage });
        if (theApp.m_general_data.IsHardwareEnable(HI_CPU) && theApp.m_cpu_temperature > 0)
            strTip += CCommon::StringFormat(_T("\r\n<%1%>: <%2%> °C"), { CCommon::LoadText(IDS_CPU_TEMPERATURE), static_cast<int>(theApp.m_cpu_temperature) });
        if (theApp.m_general_data.IsHardwareEnable(HI_GPU) && theApp.m_gpu_temperature > 0)
            strTip += CCommon::StringFormat(_T("\r\n<%1%>: <%2%> °C"), { CCommon::LoadText(IDS_GPU_TEMPERATURE), static_cast<int>(theApp.m_gpu_temperature) });
        if (theApp.m_general_data.IsHardwareEnable(HI_HDD) && theApp.m_hdd_temperature > 0)
            strTip += CCommon::StringFormat(_T("\r\n<%1%>: <%2%> °C"), { CCommon::LoadText(IDS_HDD_TEMPERATURE), static_cast<int>(theApp.m_hdd_temperature) });
        if (theApp.m_general_data.IsHardwareEnable(HI_MBD) && theApp.m_main_board_temperature > 0)
            strTip += CCommon::StringFormat(_T("\r\n<%1%>: <%2%> °C"), { CCommon::LoadText(IDS_MAINBOARD_TEMPERATURE), static_cast<int>(theApp.m_main_board_temperature) });
        if (theApp.m_general_data.IsHardwareEnable(HI_HDD) && theApp.m_hdd_usage >= 0)
            strTip += CCommon::StringFormat(_T("\r\n<%1%>: <%2%> %"), { CCommon::LoadText(IDS_HDD_USAGE), theApp.m_hdd_usage });
    }

    CCommon::WStringCopy(m_ntIcon.szTip, 128, strTip);
    ::Shell_NotifyIcon(NIM_MODIFY, &m_ntIcon);

}

void CTrafficMonitorDlg::SaveHistoryTraffic()
{
    // Background disk writes disabled
}

void CTrafficMonitorDlg::SaveHistoryTrafficFull()
{
    // Full disk writes disabled
}

void CTrafficMonitorDlg::LoadHistoryTraffic()
{
    m_history_traffic.Load();
    CHistoryTrafficFile backup_file(theApp.m_history_traffic_path + L".bak");
    backup_file.LoadSize();     //Read the number of traffic records in the backup file
    
    // If the number of traffic records in the backup file is greater than the current number, try to restore from the backup file
    if (backup_file.Size() > m_history_traffic.Size())
    {
        size_t size_before = m_history_traffic.Size();
        backup_file.Load();     //Load backup file (will clear "future" records)
        size_t backup_size_after_load = backup_file.Size();  //Actual number of records after loading (may be reduced due to cleaning of "future" records)
        
        // After loading, if the number of records in the backup file is still greater than the current file, restore it
        if (backup_size_after_load > m_history_traffic.Size())
        {
            m_history_traffic.Merge(backup_file, true);
            size_t size_after = m_history_traffic.Size();
            size_t recovered_count = size_after - size_before;  //Actual number of records recovered
            
            // Only log when the record is actually restored
            if (recovered_count > 0)
            {
                CString log_info = CCommon::LoadTextFormat(IDS_HISTORY_TRAFFIC_LOST_ERROR_LOG, { size_before, recovered_count });
                CCommon::WriteLog(log_info, theApp.m_log_path.c_str());
            }
        }
    }

    theApp.m_today_up_traffic = m_history_traffic.GetTodayUpTraffic();
    theApp.m_today_down_traffic = m_history_traffic.GetTodayDownTraffic();
}

void CTrafficMonitorDlg::BackupHistoryTrafficFile()
{
    // Backup file writes disabled
}

void CTrafficMonitorDlg::_OnOptions(int tab, CWnd* pParent)
{
    COptionsDlg optionsDlg(tab, pParent);

    //Pass option settings data to the option settings dialog box
    if (COptionsDlg::GetUniqueHandel(OPTION_DLG_NAME) == NULL)     //Make sure the Options Settings dialog box is closed at this time
    {
        optionsDlg.m_tab1_dlg.m_data = theApp.m_main_wnd_data;
        optionsDlg.m_tab2_dlg.m_data = theApp.m_taskbar_data;
        optionsDlg.m_tab3_dlg.m_data = theApp.m_general_data;
        optionsDlg.m_tab1_dlg.m_text_disable = m_skin.GetLayoutInfo().no_label;
    }

    if (optionsDlg.DoModal() == IDOK)
    {
        ApplySettings(optionsDlg);
    }
}

void CTrafficMonitorDlg::ApplySettings(COptionsDlg& optionsDlg)
{
    bool is_hardware_monitor_item_changed = (optionsDlg.m_tab3_dlg.m_data.hardware_monitor_item != theApp.m_general_data.hardware_monitor_item);
    bool is_always_on_top_changed = (optionsDlg.m_tab1_dlg.m_data.m_always_on_top != theApp.m_main_wnd_data.m_always_on_top);
    bool is_mouse_penerate_changed = (optionsDlg.m_tab1_dlg.m_data.m_mouse_penetrate != theApp.m_main_wnd_data.m_mouse_penetrate);
    bool is_alow_out_of_border_changed = (optionsDlg.m_tab1_dlg.m_data.m_alow_out_of_border != theApp.m_main_wnd_data.m_alow_out_of_border);
    bool is_show_notify_icon_changed = (optionsDlg.m_tab3_dlg.m_data.show_notify_icon != theApp.m_general_data.show_notify_icon);
    bool is_connections_hide_changed = (optionsDlg.m_tab3_dlg.m_data.connections_hide.data() != theApp.m_general_data.connections_hide.data());
    bool d2d_turned_on = (theApp.m_taskbar_data.disable_d2d && !optionsDlg.m_tab2_dlg.m_data.disable_d2d);
    //When you need to close and reopen the taskbar window again
    bool taskbar_changed = (theApp.m_taskbar_data.show_taskbar_wnd_in_secondary_display != optionsDlg.m_tab2_dlg.m_data.show_taskbar_wnd_in_secondary_display
        || theApp.m_taskbar_data.secondary_display_index != optionsDlg.m_tab2_dlg.m_data.secondary_display_index
        || theApp.m_taskbar_data.disable_d2d != optionsDlg.m_tab2_dlg.m_data.disable_d2d
        || theApp.m_taskbar_data.IsTaskbarTransparent() != optionsDlg.m_tab2_dlg.m_data.IsTaskbarTransparent()
        || theApp.m_taskbar_data.auto_set_background_color != optionsDlg.m_tab2_dlg.m_data.auto_set_background_color
        );
    bool is_skin_data_changed = (theApp.m_main_wnd_data.ToSkinSettingData() != optionsDlg.m_tab1_dlg.m_data.ToSkinSettingData());

    theApp.m_main_wnd_data = optionsDlg.m_tab1_dlg.m_data;
    theApp.m_taskbar_data = optionsDlg.m_tab2_dlg.m_data;
    theApp.m_general_data = optionsDlg.m_tab3_dlg.m_data;
    theApp.SendSettingsToPlugin();

    CGeneralSettingsDlg::CheckTaskbarDisplayItem();

    SetTextFont();

    //After turning on D2D rendering, "Background Transparency" is automatically turned on and "Automatically set background color based on taskbar color" is turned off.
    if (d2d_turned_on)
    {
        theApp.m_taskbar_data.SetTaskabrTransparent(true);
        theApp.m_taskbar_data.auto_set_background_color = false;
    }

    //CTaskBarDlg::SaveConfig();
    if (IsTaskbarWndValid())
    {
        m_tBarDlg->ApplySettings();
        //If you change the taskbar window font or displayed text, the taskbar window may change, so close and reopen the taskbar window.
        if (taskbar_changed)
        {
            CloseTaskBarWnd();
            OpenTaskBarWnd();
        }
        else
        {
            m_tBarDlg->WidthChanged();
        }
        m_tBarDlg->ApplyWindowTransparentColor();
    }

    if (optionsDlg.m_tab3_dlg.IsAutoRunModified())
    {
        if (!theApp.SetAutoRun(theApp.m_general_data.auto_run, theApp.m_general_data.auto_run_by_task_scheduler))
            MessageBox(CCommon::LoadText(IDS_SET_AUTO_RUN_FAILED_WARNING), NULL, MB_ICONWARNING | MB_OK);
    }

    if (optionsDlg.m_tab3_dlg.IsShowAllInterfaceModified() || is_connections_hide_changed)
        IniConnection();

    if (optionsDlg.m_tab3_dlg.IsMonitorTimeSpanModified())      //If the monitoring interval has changed, reset the timer
    {
        KillTimer(MONITOR_TIMER);
        SetTimer(MONITOR_TIMER, theApp.m_general_data.monitor_time_span, NULL);
    }

#ifndef WITHOUT_TEMPERATURE
    if (is_hardware_monitor_item_changed)
    {
        //If hardware monitoring is turned off, the hardware monitoring class is destroyed
        if (theApp.m_general_data.hardware_monitor_item == 0)
        {
            CSingleLock sync(&theApp.m_minitor_lib_critical, TRUE);
            theApp.m_pMonitor.reset();
        }
        else if (theApp.m_pMonitor != nullptr)
        {
            theApp.UpdateOpenHardwareMonitorEnableState();
        }
        else if (IsTemperatureNeeded())
        {
            theApp.InitOpenHardwareLibInThread();
        }
        //Update taskbar window right-click menu
        //theApp.UpdateTaskbarWndMenu();
    }
#endif

    if (is_always_on_top_changed)
    {
        SetAlwaysOnTop();
    }

    if (is_mouse_penerate_changed)
    {
        SetMousePenetrate();
        if (!theApp.m_general_data.show_notify_icon && theApp.IsForceShowNotifyIcon())   //When the mouse penetrates, if the notification icon is not displayed, display it, otherwise the right-click menu cannot be called out.
        {
            //Add notification bar icon
            AddNotifyIcon();
            theApp.m_general_data.show_notify_icon = true;
        }
    }

    if (is_alow_out_of_border_changed)
    {
        CheckWindowPos();
    }

    if (is_show_notify_icon_changed)
    {
        if (theApp.IsForceShowNotifyIcon())
            theApp.m_general_data.show_notify_icon = true;
        if (theApp.m_general_data.show_notify_icon)
            AddNotifyIcon();
        else
            DeleteNotifyIcon();
    }

    if (is_skin_data_changed)
    {
        //Save current skin settings to SkinManager
        SkinSettingData skin_data = theApp.m_main_wnd_data.ToSkinSettingData();
        CSkinManager::Instance().AddSkinSettingData(theApp.m_cfg_data.m_skin_name, skin_data);
        CSkinManager::Instance().Save();
    }

    theApp.SaveConfig();
    theApp.SaveGlobalConfig();
}

void CTrafficMonitorDlg::SetItemPosition()
{
    if (theApp.m_cfg_data.m_show_more_info)
    {
        SetWindowPos(nullptr, 0, 0, m_skin.GetLayoutInfo().layout_l.width, m_skin.GetLayoutInfo().layout_l.height, SWP_NOMOVE | SWP_NOZORDER);
    }
    else
    {
        SetWindowPos(nullptr, 0, 0, m_skin.GetLayoutInfo().layout_s.width, m_skin.GetLayoutInfo().layout_s.height, SWP_NOMOVE | SWP_NOZORDER);
    }
}

bool CTrafficMonitorDlg::LoadSkinLayout()
{
    bool rtn = m_skin.Load(CSkinManager::Instance().GetSkinName(m_skin_selected));
    if (m_skin.GetLayoutInfo().no_label)        //If the skin layout does not display text, it is not allowed to exchange the upload and download positions, because the upload and download positions are already fixed in the skin
        theApp.m_main_wnd_data.swap_up_down = false;
    return rtn;
}

void CTrafficMonitorDlg::LoadBackGroundImage()
{
    CImage img_tmp;
    CSize image_size;
    if (theApp.m_cfg_data.m_show_more_info)
    {
        image_size.SetSize(m_skin.GetLayoutInfo().layout_l.width, m_skin.GetLayoutInfo().layout_l.height);
    }
    else
    {
        image_size.SetSize(m_skin.GetLayoutInfo().layout_s.width, m_skin.GetLayoutInfo().layout_s.height);
    }

    //Create window area
    CImage img_mask;
    //Load mask image
    if (theApp.m_cfg_data.m_show_more_info)
        img_tmp.Load((theApp.m_skin_path + CSkinManager::Instance().GetSkinName(m_skin_selected) + BACKGROUND_MASK_L).c_str());
    else
        img_tmp.Load((theApp.m_skin_path + CSkinManager::Instance().GetSkinName(m_skin_selected) + BACKGROUND_MASK_S).c_str());
    CRgn wnd_rgn;
    if (!img_tmp.IsNull())
    {
        CDrawCommon::BitmapStretch(&img_tmp, &img_mask, image_size);        //Stretch mask image
        CBitmap bitmap;
        bitmap.Attach(img_mask);
        CDrawCommon::GetRegionFromImage(wnd_rgn, bitmap, 128);      //Get the area of the window from the mask image
        bitmap.Detach();
    }
    else
    {
        wnd_rgn.CreateRectRgnIndirect(CRect(CPoint(0, 0), image_size));     //If loading the mask image fails, the window size is used as the window area.
    }
    //Prevent the obtained window area from being empty
    CRgn empty_rgn;
    empty_rgn.CreateRectRgnIndirect(CRect{});   //Create an empty CRgn object
    if (wnd_rgn.EqualRgn(&empty_rgn))
        wnd_rgn.SetRectRgn(CRect(CPoint(0, 0), image_size));    //If the obtained window area is empty, use the window size as the window area.
    SetWindowRgn(wnd_rgn, TRUE);        //Set window area
    wnd_rgn.DeleteObject();
    empty_rgn.DeleteObject();
}

void CTrafficMonitorDlg::SetTextFont()
{
    m_skin.SetSettingData(theApp.m_main_wnd_data.ToSkinSettingData());
}

bool CTrafficMonitorDlg::IsTaskbarWndValid() const
{
    return m_tBarDlg != nullptr && ::IsWindow(m_tBarDlg->GetSafeHwnd());
}

void CTrafficMonitorDlg::TaskbarShowHideItem(DisplayItem type)
{
    if (IsTaskbarWndValid())
    {
        bool show = (theApp.m_taskbar_data.display_item.Contains(type));
        if (show)
        {
            theApp.m_taskbar_data.display_item.Remove(type);
        }
        else
        {
            theApp.m_taskbar_data.display_item.Add(type);
        }
        //CloseTaskBarWnd();
        //OpenTaskBarWnd();
        m_tBarDlg->WidthChanged();
    }
}

void CTrafficMonitorDlg::CheckClickedItem(CPoint point)
{
    const CSkinFile::Layout& skin_layout{ theApp.m_cfg_data.m_show_more_info ? m_skin.GetLayoutInfo().layout_l : m_skin.GetLayoutInfo().layout_s }; //Current skin layout
    for (const auto& layout_item : skin_layout.layout_items)
    {
        CRect rect(CPoint(layout_item.second.x, layout_item.second.y), CSize(layout_item.second.width, m_skin.GetLayoutInfo().text_height));
        if (rect.PtInRect(point))
        {
            m_clicked_item = layout_item.first;
            break;
        }
    }
}

void CTrafficMonitorDlg::ApplySkin(int skin_index)
{
    if (skin_index < 0 || skin_index >= CSkinManager::Instance().Size())
        return;
    m_skin_selected = skin_index;
    theApp.m_cfg_data.m_skin_name = CSkinManager::Instance().GetSkinName(m_skin_selected);
    //Get skin layout
    LoadSkinLayout();
    //Load background image
    LoadBackGroundImage();
    //Get skin settings
    SkinSettingData cur_skin_data;
    if (!CSkinManager::Instance().GetSkinSettingDataByIndex(m_skin_selected, cur_skin_data))
    {
        CSkinManager::SkinSettingDataFronSkin(cur_skin_data, m_skin);
    }
    theApp.m_main_wnd_data.FormSkinSettingData(cur_skin_data);
    SetTextFont();
    SetItemPosition();
    Invalidate(FALSE);      //Immediately refresh the window information after changing the skin
    //Reset the WS_EX_LAYERED style to solve the problem of abnormal display when switching between png skin and bmp skin
    //Clear layered styles for windows
    SetWindowLong(m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
    //Reset transparency
    SetTransparency();

    theApp.SaveConfig();
}

bool CTrafficMonitorDlg::IsTemperatureNeeded() const
{
    //Determine whether information needs to be obtained from OpenHardwareMonitor.
    ////Return true only if at least one of the CPU temperature, graphics card utilization, graphics card temperature, hard disk temperature and motherboard temperature needs to be displayed in the main window and taskbar window
    //bool needed = false;
    //if (theApp.m_cfg_data.m_show_task_bar_wnd && IsTaskbarWndValid())
    //{
    //    needed |= m_tBarDlg->IsShowCpuTemperature();
    //    needed |= m_tBarDlg->IsShowGpu();
    //    needed |= m_tBarDlg->IsShowGpuTemperature();
    //    needed |= m_tBarDlg->IsShowHddTemperature();
    //    needed |= m_tBarDlg->IsShowMainboardTemperature();
    //    needed |= (::IsWindow(m_tBarDlg->m_tool_tips.GetSafeHwnd()) && m_tBarDlg->m_tool_tips.IsWindowVisible());
    //}

    //if (!theApp.m_cfg_data.m_hide_main_window)
    //{
    //    const CSkinFile::Layout& skin_layout{ theApp.m_cfg_data.m_show_more_info ? m_skin.GetLayoutInfo().layout_l : m_skin.GetLayoutInfo().layout_s }; //Current skin layout
    //    needed |= skin_layout.GetItem(TDI_CPU_TEMP).show;
    //    needed |= skin_layout.GetItem(TDI_GPU_USAGE).show;
    //    needed |= skin_layout.GetItem(TDI_GPU_TEMP).show;
    //    needed |= skin_layout.GetItem(TDI_HDD_TEMP).show;
    //    needed |= skin_layout.GetItem(TDI_MAIN_BOARD_TEMP).show;
    //    needed |= (::IsWindow(m_tool_tips.GetSafeHwnd()) && m_tool_tips.IsWindowVisible());
    //}
    //return needed;

    return theApp.m_general_data.IsHardwareEnable(HI_CPU) || theApp.m_general_data.IsHardwareEnable(HI_GPU)
        || theApp.m_general_data.IsHardwareEnable(HI_HDD) || theApp.m_general_data.IsHardwareEnable(HI_MBD);
}

// CTrafficMonitorDlg message handler

BOOL CTrafficMonitorDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Sets the icon for this dialog box.  When the application's main window is not a dialog box, the frame will automatically
    //  do this
    SetIcon(m_hIcon, TRUE);         // Set large icons
    SetIcon(m_hIcon, FALSE);        // Set small icon

    // TODO: Add additional initialization code here
    SetWindowText(APP_NAME);
    //Set to hide taskbar icons
    ModifyStyleEx(WS_EX_APPWINDOW, WS_EX_TOOLWINDOW);

    theApp.DPIFromWindow(this);
    //Get screen size
    GetScreenSize();
    m_last_screen_rects = m_screen_rects;
    //::SystemParametersInfo(SPI_GETWORKAREA, 0, &m_screen_rect, 0); // Get the work area size

    //Initialization menu
    theApp.InitMenuResourse();
    //theApp.UpdateTaskbarWndMenu();

    //Initialize skin
    CSkinManager::Instance().Init();
    m_skin_selected = CSkinManager::Instance().FindSkinIndex(theApp.m_cfg_data.m_skin_name);

    //Get layout data based on the currently selected skin
    if (LoadSkinLayout())
    {
        //Get the current skin's settings from SkinManager
        SkinSettingData cur_skin_data;
        CSkinManager::SkinSettingDataFronSkin(cur_skin_data, m_skin);   //Get the skin's default settings
        CSkinManager::Instance().GetSkinSettingDataByIndex(m_skin_selected, cur_skin_data); //Get the skin's user-saved data
        theApp.m_main_wnd_data.FormSkinSettingData(cur_skin_data);
    }

    //Set window transparency
    SetTransparency();

    IniConnection();    //Initialize connection

    //If mouse penetration is set at startup or the main window is hidden, and the taskbar window is not displayed, the notification area icon is displayed.
    if ((theApp.m_main_wnd_data.m_mouse_penetrate || theApp.m_cfg_data.m_hide_main_window) && !theApp.m_cfg_data.m_show_task_bar_wnd)
        theApp.m_general_data.show_notify_icon = true;

    //Load notification area icon
    theApp.m_notify_icons[0] = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_NOFITY_ICON), IMAGE_ICON, theApp.DPI(16), theApp.DPI(16), LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);
    theApp.m_notify_icons[1] = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_NOFITY_ICON2), IMAGE_ICON, theApp.DPI(16), theApp.DPI(16), LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);
    theApp.m_notify_icons[2] = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_NOFITY_ICON3), IMAGE_ICON, theApp.DPI(16), theApp.DPI(16), LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);
    theApp.m_notify_icons[3] = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, theApp.DPI(16), theApp.DPI(16), LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);
    theApp.m_notify_icons[4] = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_NOFITY_ICON4), IMAGE_ICON, theApp.DPI(16), theApp.DPI(16), LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);
    theApp.m_notify_icons[5] = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_NOTIFY_ICON5), IMAGE_ICON, theApp.DPI(16), theApp.DPI(16), LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);

    //Set notification area icon
    m_ntIcon.cbSize = sizeof(NOTIFYICONDATA);   //The size of the structure variable
    if (theApp.m_cfg_data.m_notify_icon_selected < 0 || theApp.m_cfg_data.m_notify_icon_selected >= MAX_NOTIFY_ICON)
        theApp.m_cfg_data.m_notify_icon_selected = 0;
    m_ntIcon.hIcon = theApp.m_notify_icons[theApp.m_cfg_data.m_notify_icon_selected];       //settings icon
    m_ntIcon.hWnd = this->m_hWnd;               //Window handle to receive tray icon notification messages
    CString atip;           //Tips displayed when the mouse points to the icon
#ifdef _DEBUG
    atip = CCommon::LoadText(IDS_TRAFFICMONITOR, _T(" (Debug)"));
#else
    atip = CCommon::LoadText(IDS_TRAFFICMONITOR);
#endif
    //wcscpy_s(m_ntIcon.szTip, 128, strTip);
    CCommon::WStringCopy(m_ntIcon.szTip, 128, atip.GetString());
    m_ntIcon.uCallbackMessage = MY_WM_NOTIFYICON;   //Application-defined message ID number
    m_ntIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP; //Icon properties: Setting members uCallbackMessage, hIcon, and szTip is valid
    if (theApp.m_general_data.show_notify_icon)
        ::Shell_NotifyIcon(NIM_ADD, &m_ntIcon); //Add this icon to the system notification area

    //Load traffic history
    LoadHistoryTraffic();

    //Set a timer that triggers in 1000 milliseconds
    SetTimer(MAIN_TIMER, 1000, NULL);

    SetTimer(MONITOR_TIMER, theApp.m_general_data.monitor_time_span, NULL);
    AfxBeginThread(MonitorThreadCallback, (LPVOID)this);

    //Initialize window position
    SetItemPosition();
    if (theApp.m_cfg_data.m_position_x != -1 && theApp.m_cfg_data.m_position_y != -1)
        SetWindowPos(nullptr, theApp.m_cfg_data.m_position_x, theApp.m_cfg_data.m_position_y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
    CheckWindowPos();

    //Load background image
    LoadBackGroundImage();

    //Set font
    SetTextFont();

    //Get the time at startup
    GetLocalTime(&m_start_time);

    //Initialize mouse tips
    m_tool_tips.Create(this, TTS_ALWAYSTIP);
    m_tool_tips.SetMaxTipWidth(600);
    m_tool_tips.SetDelayTime(TTDT_INITIAL, 1200);   // Hover delay 1.2s
    m_tool_tips.SetDelayTime(TTDT_AUTOPOP, 10000);  // Keep open for 10s
    m_tool_tips.SetDelayTime(TTDT_RESHOW, 1200);
    m_tool_tips.AddTool(this, _T(""));

    //If the main window is set to be hidden when the program is started, or the window is located in the upper left corner, first set its opacity to 0
    if (theApp.m_cfg_data.m_hide_main_window || (theApp.m_cfg_data.m_position_x == 0 && theApp.m_cfg_data.m_position_y == 0))
        SetTransparency(0);

    SetTimer(TASKBAR_TIMER, 100, NULL);

    return TRUE;  // Returns TRUE unless focus is set to the control
}


//When the user drags the minimized window, the system calls this function to obtain the cursor.
//display.
HCURSOR CTrafficMonitorDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

//Calculate the number of times the Monitor timer triggers within the specified number of seconds
static int GetMonitorTimerCount(int second)
{
    int count = second * 1000 / theApp.m_general_data.monitor_time_span;
    if (count <= 0) count = 1;
    return count;
}


void CTrafficMonitorDlg::DoMonitorAcquisition()
{
    //Get network connection speed
    int rtn{};
    auto getLfTable = [&]() {
        __try
        {
            rtn = GetIfTable(m_pIfTable, &m_dwSize, FALSE);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            free(m_pIfTable);
            m_dwSize = sizeof(MIB_IFTABLE);
            m_pIfTable = (MIB_IFTABLE*)malloc(m_dwSize);
            rtn = GetIfTable(m_pIfTable, &m_dwSize, FALSE);
            if (rtn == ERROR_INSUFFICIENT_BUFFER)	//If the function return value is ERROR_INSUFFICIENT_BUFFER, it means that the size of m_pIfTable is not enough.
            {
                free(m_pIfTable);
                m_pIfTable = (MIB_IFTABLE*)malloc(m_dwSize);	//Re-open a piece of memory with a new size
            }
            GetIfTable(m_pIfTable, &m_dwSize, FALSE);
        }
    };

    getLfTable();

    if (!theApp.m_cfg_data.m_select_all)        //Get the network speed of the currently selected connection
    {
        auto table = GetConnectIfTable(m_connection_selected);
        m_in_bytes = table.dwInOctets;
        m_out_bytes = table.dwOutOctets;
    }
    else        //Get the speed of all connections
    {
        m_in_bytes = 0;
        m_out_bytes = 0;
        for (size_t i{}; i < m_connections.size(); i++)
        {
            auto table = GetConnectIfTable(i);
            //if (i > 0 && m_pIfTable->table[m_connections[i].index].dwInOctets == m_pIfTable->table[m_connections[i - 1].index].dwInOctets
            //  && m_pIfTable->table[m_connections[i].index].dwOutOctets == m_pIfTable->table[m_connections[i - 1].index].dwOutOctets)
            //  continue; //There may be the same connection in the connection list. When counting the network speed of all connections, connections with exactly the same number of bytes sent and received are ignored.
            m_in_bytes += table.dwInOctets;
            m_out_bytes += table.dwOutOctets;
        }
    }

    unsigned __int64 cur_in_speed{}, cur_out_speed{};       //Upload and download speeds during this monitoring time interval

    //If the number of bytes sent and received is 0 or the last number of bytes sent and received is 0 or the current connection has changed, the network speed is invalid.
    if ((m_in_bytes == 0 && m_out_bytes == 0) || (m_last_in_bytes == 0 && m_last_out_bytes == 0) || m_connection_change_flag
        || m_last_in_bytes > m_in_bytes || m_last_out_bytes > m_out_bytes)
    {
        cur_in_speed = 0;
        cur_out_speed = 0;
    }
    else
    {
        cur_in_speed = m_in_bytes - m_last_in_bytes;
        cur_out_speed = m_out_bytes - m_last_out_bytes;
    }
    ////If it is greater than 1GB/s, it means that an exception may have occurred and the network speed is invalid.
    //if (cur_in_speed > 1073741824)
    //  cur_in_speed = 0;
    //if (cur_out_speed > 1073741824)
    //  cur_out_speed = 0;

    //Calculate the time interval between two acquisitions of network speed
    static ULONGLONG last_net_speed_time = 0;
    ULONGLONG net_speed_time = CCommon::GetCurrentTimeSinceEpochMilliseconds();
    int time_span = theApp.m_general_data.monitor_time_span;
    if (last_net_speed_time != 0)
        time_span = static_cast<int>(net_speed_time - last_net_speed_time);
    last_net_speed_time = net_speed_time;

    //Convert the traffic of the current monitoring interval into the traffic of every second interval
    theApp.m_in_speed = static_cast<unsigned __int64>(cur_in_speed * 1000 / time_span);
    theApp.m_out_speed = static_cast<unsigned __int64>(cur_out_speed * 1000 / time_span);

    m_connection_change_flag = false;    //Clear the flag that the connection has changed

    m_last_in_bytes = m_in_bytes;
    m_last_out_bytes = m_out_bytes;

    //When in the automatic selection state, if there is no network speed for 30 consecutive seconds, the automatically selected network may be incorrect. In this case, perform an automatic selection.
    if (theApp.m_cfg_data.m_auto_select)
    {
        if (cur_in_speed == 0 && cur_out_speed == 0)
            m_zero_speed_cnt++;
        else
            m_zero_speed_cnt = 0;
        if (m_zero_speed_cnt >= GetMonitorTimerCount(30))
        {
            AutoSelect();
            m_zero_speed_cnt = 0;
        }
    }

    //Check whether the current date has changed. If it has changed, insert a new date into the historical traffic list.
    SYSTEMTIME current_time;
    GetLocalTime(&current_time);
    static int last_check_day = -1;  //Used to detect date changes and reset the saved state
    if (m_history_traffic.GetTodayTraffic().day != current_time.wDay)
    {
        m_history_traffic.OnDateChanged();
        theApp.m_today_up_traffic = 0;
        theApp.m_today_down_traffic = 0;
        last_check_day = -1;  //Resets the date stamp and the saved state will be reinitialized the next time it is checked
    }

    //Statistics of traffic used today
    theApp.m_today_up_traffic += cur_out_speed;
    theApp.m_today_down_traffic += cur_in_speed;
    m_history_traffic.GetTodayTraffic().up_kBytes = theApp.m_today_up_traffic / 1024u;
    m_history_traffic.GetTodayTraffic().down_kBytes = theApp.m_today_down_traffic / 1024u;
    //Save traffic history every 30 seconds
    if (m_monitor_time_cnt % GetMonitorTimerCount(30) == GetMonitorTimerCount(30) - 1)
    {
        static unsigned __int64 last_today_kbytes = 0;
        static bool last_today_kbytes_initialized = false;
        unsigned __int64 current_kbytes = m_history_traffic.GetTodayTraffic().kBytes();
        
        //If the date has changed, reset the initialization state
        if (last_check_day != current_time.wDay)
        {
            last_today_kbytes_initialized = false;
            last_check_day = current_time.wDay;
        }
        
        //Initialized on first check, not saved
        if (!last_today_kbytes_initialized)
        {
            last_today_kbytes = current_kbytes;
            last_today_kbytes_initialized = true;
        }
        else
        {
            //Historical traffic records are only saved when the traffic change exceeds 10MB within 30 seconds to prevent disk writes from being written too frequently.
            unsigned __int64 change_kbytes = current_kbytes - last_today_kbytes;
            if (change_kbytes >= 10240u) // 10MB = 10240KB
            {
                SaveHistoryTraffic();
                last_today_kbytes = current_kbytes;
            }
        }
    }

    if (rtn == ERROR_INSUFFICIENT_BUFFER)
    {
        IniConnection();
        CString info = CCommon::LoadText(IDS_INSUFFICIENT_BUFFER);
        info.Replace(_T("<%cnt%>"), CCommon::IntToString(m_restart_cnt));
        CCommon::WriteLog(info, theApp.m_log_path.c_str());
    }

    if (m_monitor_time_cnt % GetMonitorTimerCount(3) == GetMonitorTimerCount(3) - 1)
    {
        //Retrieve the current number of connections
        static DWORD last_interface_num = -1;
        DWORD interface_num;
        GetNumberOfInterfaces(&interface_num);
        if (last_interface_num != -1 && interface_num != last_interface_num)    //If the number of connections changes, reinitialize the connections
        {
            if (theApp.m_debug_log)
            {
                CString info = CCommon::LoadText(IDS_CONNECTION_NUM_CHANGED);
                info.Replace(_T("<%before%>"), CCommon::IntToString(last_interface_num));
                info.Replace(_T("<%after%>"), CCommon::IntToString(interface_num));
                info.Replace(_T("<%cnt%>"), CCommon::IntToString(m_restart_cnt + 1));
                CCommon::WriteLog(info, theApp.m_log_path.c_str());
            }
            IniConnection();
            last_interface_num = interface_num;
        }

        string descr;
        descr = (const char*)GetConnectIfTable(m_connection_selected).bDescr;
        if (descr != theApp.m_cfg_data.m_connection_name)
        {
            //Write additional debugging information
            if (theApp.m_debug_log)
            {
                CString log_str;
                log_str = _T("Connection name does not match:\r\n");
                log_str += _T("IfTable description: ");
                log_str += descr.c_str();
                log_str += _T("\r\nm_connection_name: ");
                log_str += theApp.m_cfg_data.m_connection_name.c_str();
                CCommon::WriteLog(log_str, (theApp.m_config_dir + L".\\connections.log").c_str());
            }

            IniConnection();
            CString info = CCommon::LoadText(IDS_CONNECTION_NOT_MATCH);
            info.Replace(_T("<%cnt%>"), CCommon::IntToString(m_restart_cnt));
            CCommon::WriteLog(info, theApp.m_log_path.c_str());
        }
    }

    bool lite_version = false;
#ifdef WITHOUT_TEMPERATURE
    lite_version = true;
#endif

    bool cpu_freq_acquired = false;
    bool gpu_usage_acquired = false;
    m_get_disk_usage_by_pdh = false;

    //Get CPU usage
    theApp.m_cpu_usage = m_cpu_usage_helper.GetCpuUsage(theApp.m_general_data.cpu_usage_acquire_method == GeneralSettingData::CA_CPU_TIME);

    //Get CPU frequency
    //if (lite_version || is_arm64ec || !theApp.m_general_data.IsHardwareEnable(HI_CPU))
    //{
    if (m_cpu_freq_helper.GetCpuFreq(theApp.m_cpu_freq))
        cpu_freq_acquired = true;
    //}

    //Get GPU utilization
    if (lite_version /*|| is_arm64ec*/ || !theApp.m_general_data.IsHardwareEnable(HI_GPU))
    {
        if (m_gpu_usage_helper.GetGpuUsage(theApp.m_gpu_usage))
            gpu_usage_acquired = true;
        else
            theApp.m_gpu_usage = -1;
    }

    //Get hard drive utilization
    if (lite_version /*|| is_arm64ec*/ || !theApp.m_general_data.IsHardwareEnable(HI_HDD))
    {
        int disk_index = m_disk_usage_helper.FindDiskIndex(theApp.m_general_data.hard_disk_name);
        //When no hard disk is found to be monitored, the overall utilization is used by default.
        if (disk_index < 0)
        {
            disk_index = m_disk_usage_helper.FindDiskIndex(L"_Total");
            if (disk_index >= 0)
            {
                theApp.m_general_data.hard_disk_name = L"_Total";
            }
            //Still not found using the 1st hard drive
            else
            {
                const auto& disk_names = m_disk_usage_helper.GetDiskNames();
                if (!disk_names.empty())
                {
                    disk_index = 0;
                    theApp.m_general_data.hard_disk_name = disk_names.front();
                }
            }
        }
        if (m_disk_usage_helper.GetDiskUsage(disk_index, theApp.m_hdd_usage))
            m_get_disk_usage_by_pdh = true;
        else
            theApp.m_hdd_usage = -1;
    }

    //Get memory utilization
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    theApp.m_memory_usage = statex.dwMemoryLoad;
    theApp.m_used_memory = static_cast<int>((statex.ullTotalPhys - statex.ullAvailPhys) / 1024);
    theApp.m_total_memory = static_cast<int>(statex.ullTotalPhys / 1024);

#ifndef WITHOUT_TEMPERATURE
    //Get temperature
    // 1. Native Windows 11 Driverless CPU Temperature (ACPI / Thermal Zone)
    if (IsTemperatureNeeded())
    {
        float pdh_temp = 0.0f;
        if (m_cpu_temp_helper.GetCpuTemperature(pdh_temp) && pdh_temp > 0)
        {
            theApp.m_cpu_temperature = pdh_temp;
        }
    }

    // 2. Hardware telemetry (GPU via user-mode NvAPI / storage via SMART)
    if (IsTemperatureNeeded() && theApp.m_pMonitor != nullptr)
    {
        CSingleLock sync(&theApp.m_minitor_lib_critical, TRUE);
        CString error_info = CCommon::LoadText(IDS_HARDWARE_INFO_ACQUIRE_FAILED_ERROR);

        auto getHardwareInfo = [&]() {
            __try
            {
                theApp.m_pMonitor->GetHardwareInfo();
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                if (theApp.m_debug_log)
                    CCommon::WriteLog(error_info, theApp.m_log_path.c_str());
            }
        };

        getHardwareInfo();
        auto monitor_error_message{ OpenHardwareMonitorApi::GetErrorMessage() };
        if (!monitor_error_message.empty() && theApp.m_debug_log)
        {
            CCommon::WriteLog(monitor_error_message.c_str(), theApp.m_log_path.c_str());
        }
        theApp.m_gpu_temperature = theApp.m_pMonitor->GpuTemperature();
        theApp.m_main_board_temperature = theApp.m_pMonitor->MainboardTemperature();
        if (!gpu_usage_acquired)
            theApp.m_gpu_usage = theApp.m_pMonitor->GpuUsage();
        if (!cpu_freq_acquired)
            theApp.m_cpu_freq = theApp.m_pMonitor->CpuFreq();

        //Get hard drive temperature
        if (!theApp.m_pMonitor->AllHDDTemperature().empty())
        {
            auto iter = theApp.m_pMonitor->AllHDDTemperature().find(theApp.m_general_data.hard_disk_name);
            if (iter == theApp.m_pMonitor->AllHDDTemperature().end())
            {
                iter = theApp.m_pMonitor->AllHDDTemperature().begin();
                theApp.m_general_data.hard_disk_name = iter->first;
            }
            theApp.m_hdd_temperature = iter->second;
        }
        else
        {
            theApp.m_hdd_temperature = -1;
        }
        //Get hard drive utilization
        if (!m_get_disk_usage_by_pdh)
        {
            if (!theApp.m_pMonitor->AllHDDUsage().empty())
            {
                auto iter = theApp.m_pMonitor->AllHDDUsage().find(theApp.m_general_data.hard_disk_name);
                if (iter == theApp.m_pMonitor->AllHDDUsage().end())
                {
                    iter = theApp.m_pMonitor->AllHDDUsage().begin();
                    theApp.m_general_data.hard_disk_name = iter->first;
                }
                theApp.m_hdd_usage = iter->second;
            }
            else
            {
                theApp.m_hdd_usage = -1;
            }
        }
    }

    // 3. Integrated GPU (iGPU) temperature fallback
    // In iGPU mode, the discrete GPU is disabled. The integrated GPU is located on the
    // CPU die and shares the CPU package thermal envelope. If no standalone
    // GPU temperature sensor is reported by hardware APIs, mirror the CPU package temperature.
    if (IsTemperatureNeeded() && theApp.m_gpu_temperature <= 0 && theApp.GetGpuMode() == CTrafficMonitorApp::GpuMode::IGPU && theApp.m_cpu_temperature > 0)
    {
        theApp.m_gpu_temperature = theApp.m_cpu_temperature;
    }
#endif

    //Notify the plug-in to obtain data and pass monitoring data to the plug-in
    for (const auto& plugin_info : theApp.m_plugins.GetPlugins())
    {
        if (plugin_info.plugin != nullptr)
        {
            plugin_info.plugin->DataRequired();
            ITMPlugin::MonitorInfo monitor_info;
            monitor_info.up_speed = theApp.m_out_speed;
            monitor_info.down_speed = theApp.m_in_speed;
            monitor_info.cpu_usage = theApp.m_cpu_usage;
            monitor_info.memory_usage = theApp.m_memory_usage;
            monitor_info.gpu_usage = theApp.m_gpu_usage;
            monitor_info.hdd_usage = theApp.m_hdd_usage;
            monitor_info.cpu_temperature = theApp.m_cpu_temperature;
            monitor_info.gpu_temperature = theApp.m_gpu_temperature;
            monitor_info.hdd_temperature = theApp.m_hdd_temperature;
            monitor_info.cpu_freq = theApp.m_cpu_freq;
            monitor_info.main_board_temperature = theApp.m_main_board_temperature;
            plugin_info.plugin->OnMonitorInfo(monitor_info);
        }
    }

    m_monitor_time_cnt++;

    //Send monitoring information update message
    SendMessage(WM_MONITOR_INFO_UPDATED);
}

UINT CTrafficMonitorDlg::MonitorThreadCallback(LPVOID dwUser)
{
    CTrafficMonitorDlg* pThis = (CTrafficMonitorDlg*)dwUser;
    while (true)
    {
        //Get monitoring data once
        if (pThis->m_monitor_data_required)
        {
            pThis->DoMonitorAcquisition();
            //Reset the flag after obtaining monitoring data
            pThis->m_monitor_data_required = false;
        }
        else
        {
            Sleep(10);
        }

        // Check exit flag
        if (pThis->m_is_thread_exit)
        {
            // Trigger an event to notify the main thread that the worker thread has exited
            pThis->m_threadExitEvent.SetEvent();
            return 0;
        }
    }

    return 0;
}


void CTrafficMonitorDlg::ExitMonitorThread()
{
    // Notify thread to exit
    m_is_thread_exit = true;

    // Wait for thread to exit
    ::WaitForSingleObject(m_threadExitEvent.m_hObject, 1000);
}


void CTrafficMonitorDlg::OnTimer(UINT_PTR nIDEvent)
{
    // TODO: Add message handler code and/or call defaults here
    if (nIDEvent == MONITOR_TIMER)
    {
        //Notify the thread to obtain monitoring data
        m_monitor_data_required = true;
    }

    if (nIDEvent == MAIN_TIMER)
    {
        m_timer_cnt++;
        if (m_first_start)      //This if statement is executed 1 second after the program starts
        {
            //Putting the setting window on top is here to solve the problem
            //Putting it in the initialization function may cause the problem of invalid setting.
            SetAlwaysOnTop();       //Set window to top
            SetMousePenetrate();    //Set mouse penetration
            if (theApp.m_cfg_data.m_hide_main_window)   //Set to hide the main window
                ShowWindow(SW_HIDE);

            //Open taskbar window
            if (theApp.m_cfg_data.m_show_task_bar_wnd && m_tBarDlg == nullptr)
                OpenTaskBarWnd();

            //If the position of the window is (0, 0), the MoveWindow function is invalid during initialization. At this time, move the window again.
            if (theApp.m_cfg_data.m_position_x == 0 && theApp.m_cfg_data.m_position_y == 0)
            {
                SetWindowPos(nullptr, theApp.m_cfg_data.m_position_x, theApp.m_cfg_data.m_position_y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
            }
            SetTransparency();              //Reset window opacity

            //Check whether the DPI at the location of the main window has changed
            UINT dpi_x, dpi_y;
            CRect rect;
            GetWindowRect(rect);
            bool rtn = theApp.DPIFromRect(rect, & dpi_x, & dpi_y);
            int dpi = static_cast<int>(dpi_x);
            if (rtn && dpi != theApp.GetDpi())
            {
                theApp.SetDPI(dpi_x);
                LoadSkinLayout();   //Get layout data based on the currently selected skin
                SetItemPosition();  //Initialize window position
                LoadBackGroundImage();
                SetTextFont();      //Reload font
                Invalidate(FALSE);  //Redraw interface
            }

            m_first_start = false;
        }

        if (theApp.m_main_wnd_data.m_always_on_top && !theApp.m_cfg_data.m_hide_main_window)
        {
            //Determine whether the foreground window is full screen every 1 second
            CRect rect;
            GetWindowRect(rect);
            HMONITOR h_current_monitor = ::MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
            m_is_foreground_fullscreen = CCommon::IsForegroundFullscreen(h_current_monitor);
            if (theApp.m_main_wnd_data.hide_main_wnd_when_fullscreen)       //When full screen is set to hide the floating window
            {
                if (m_is_foreground_fullscreen || theApp.m_cfg_data.m_hide_main_window)
                    ShowWindow(SW_HIDE);
                else
                    ShowWindow(SW_RESTORE);
            }
            else            //If the floating window is not set to be hidden in full screen, then if a program enters the full screen state, set the window to be on top.
            {
                static bool last_foreground_fullscreen;
                if (!last_foreground_fullscreen && m_is_foreground_fullscreen)
                {
                    SetAlwaysOnTop();
                }
                last_foreground_fullscreen = m_is_foreground_fullscreen;
            }
        }

        if (!m_menu_popuped)
        {
            //A few seconds after the program starts, re-execute the operations of "Always on Top", "Mouse Penetration" and "Hide Main Window" according to the settings to prevent the settings from not taking effect.
            if (m_timer_cnt == 5 || m_timer_cnt == 9)
            {
                if (!theApp.m_cfg_data.m_hide_main_window)
                {
                    SetAlwaysOnTop();
                    SetMousePenetrate();
                }
                else
                {
                    ShowWindow(SW_HIDE);
                }
            }

            if (m_timer_cnt % 300 == 299 && CBaseDialog::IsAllDialogClosed() && !theApp.m_cfg_data.m_hide_main_window && theApp.m_main_wnd_data.m_always_on_top)
            {
                SetAlwaysOnTop();       //Execute setting window top every 5 minutes
            }
        }
        else
        {
            m_tool_tips.Pop();          //When the right-click menu is displayed, the mouse prompt is not displayed
        }

        if (m_timer_cnt % 30 == 26)     //Detect the window position every 30 seconds and save the settings when the window position changes
        {
            static int last_pos_x{ -1 }, last_pos_y{ -1 };
            if (last_pos_x != theApp.m_cfg_data.m_position_x || last_pos_y != theApp.m_cfg_data.m_position_y)
            {
                theApp.SaveConfig();
                last_pos_x = theApp.m_cfg_data.m_position_x;
                last_pos_y = theApp.m_cfg_data.m_position_y;
            }
        }

        if (m_timer_cnt % 2 == 1)       //Get the screen area every 2 seconds
        {
            GetScreenSize();
            CheckWindowPos();
        }

        //The following processing is performed only when at least one of the main window and the taskbar window is displayed.
        if (!theApp.m_cfg_data.m_hide_main_window || theApp.m_cfg_data.m_show_task_bar_wnd)
        {
            //Check whether the taskbar can be embedded every 10 seconds
            if (!theApp.m_win_version.IsWine() && IsTaskbarWndValid() && m_timer_cnt % 10 == 1)
            {
                if (m_tBarDlg->GetCannotInsertToTaskBar() && m_insert_to_taskbar_cnt < MAX_INSERT_TO_TASKBAR_CNT)
                {
                    CloseTaskBarWnd();
                    OpenTaskBarWnd();
                    m_insert_to_taskbar_cnt++;
                    if (m_tBarDlg->GetCannotInsertToTaskBar() && m_insert_to_taskbar_cnt >= WARN_INSERT_TO_TASKBAR_CNT)
                    {
                        //Write error log
                        CString info = CCommon::LoadText(IDS_CONNOT_INSERT_TO_TASKBAR_ERROR_LOG);
                        info.Replace(_T("<%cnt%>"), CCommon::IntToString(m_insert_to_taskbar_cnt));
                        info.Replace(_T("<%error_code%>"), CCommon::IntToString(m_tBarDlg->GetErrorCode()));
                        CCommon::WriteLog(info, theApp.m_log_path.c_str());
                        if (m_cannot_insert_to_task_bar_warning)      //Make sure the prompt message only pops up once
                        {
                            //Pop up error message
                            m_cannot_insert_to_task_bar_warning = false;
                            MessageBox(CCommon::LoadText(IDS_CONNOT_INSERT_TO_TASKBAR, CCommon::IntToString(m_tBarDlg->GetErrorCode())), NULL, MB_ICONWARNING);
                        }
                    }
                }
                if (!m_tBarDlg->GetCannotInsertToTaskBar())
                {
                    m_insert_to_taskbar_cnt = 0;
                }
            }
        }

        //Check whether you need to pop up mouse prompts
        //setting_data: setting data of message prompt
        //value: current value
        //last_value: Pass a static or variable that can continue to exist after the lambda expression is called to save the last value.
        //notify_time: Pass a static or variable that can continue to exist after the lambda expression is called, used to record the time when the last prompt popped up (the number of timer triggers)
        //tip_str: message to be prompted
        auto checkNotifyTip = [&](GeneralSettingData::NotifyTipSettings setting_data, int value, int& last_value, int& notify_time, LPCTSTR tip_str)
        {
            if (setting_data.enable)
            {
                if (last_value < setting_data.tip_value && value >= setting_data.tip_value && (m_timer_cnt - notify_time > static_cast<unsigned int>(theApp.m_notify_interval)))
                {
                    ShowNotifyTip(CCommon::LoadText(_T("TrafficMonitor "), IDS_NOTIFY, _T("")), tip_str);
                    notify_time = m_timer_cnt;
                }
                last_value = value;
            }
        };

        //Check whether you want to pop up the memory usage exceeded prompt
        CString info;
        info.Format(CCommon::LoadText(IDS_MEMORY_UDAGE_EXCEED, _T(" %d%%!")), theApp.m_memory_usage);
        static int last_memory_usage;
        static int memory_usage_notify_time{ -theApp.m_notify_interval };       //Record the time when the prompt popped up last time
        checkNotifyTip(theApp.m_general_data.memory_usage_tip, theApp.m_memory_usage, last_memory_usage, memory_usage_notify_time, info.GetString());

        //Check whether you want to pop up the CPU temperature usage exceeded prompt
        info.Format(CCommon::LoadText(IDS_CPU_TEMPERATURE_EXCEED, _T(" %d°C!")), static_cast<int>(theApp.m_cpu_temperature));
        static int last_cpu_temp;
        static int cpu_temp_notify_time{ -theApp.m_notify_interval };       //Record the time when the prompt popped up last time
        checkNotifyTip(theApp.m_general_data.cpu_temp_tip, theApp.m_cpu_temperature, last_cpu_temp, cpu_temp_notify_time, info.GetString());

        //Check whether you want to pop up the graphics card temperature usage exceeded prompt
        info.Format(CCommon::LoadText(IDS_GPU_TEMPERATURE_EXCEED, _T(" %d°C!")), static_cast<int>(theApp.m_gpu_temperature));
        static int last_gpu_temp;
        static int gpu_temp_notify_time{ -theApp.m_notify_interval };       //Record the time when the prompt popped up last time
        checkNotifyTip(theApp.m_general_data.gpu_temp_tip, theApp.m_gpu_temperature, last_gpu_temp, gpu_temp_notify_time, info.GetString());

        //Check whether you want to pop up the hard disk temperature usage exceeded prompt
        info.Format(CCommon::LoadText(IDS_HDD_TEMPERATURE_EXCEED, _T(" %d°C!")), static_cast<int>(theApp.m_hdd_temperature));
        static int last_hdd_temp;
        static int hdd_temp_notify_time{ -theApp.m_notify_interval };       //Record the time when the prompt popped up last time
        checkNotifyTip(theApp.m_general_data.hdd_temp_tip, theApp.m_hdd_temperature, last_hdd_temp, hdd_temp_notify_time, info.GetString());

        //Check whether you want to pop up the motherboard temperature usage exceeded prompt
        info.Format(CCommon::LoadText(IDS_MBD_TEMPERATURE_EXCEED, _T(" %d°C!")), static_cast<int>(theApp.m_main_board_temperature));
        static int last_main_board_temp;
        static int main_board_temp_notify_time{ -theApp.m_notify_interval };        //Record the time when the prompt popped up last time
        checkNotifyTip(theApp.m_general_data.mainboard_temp_tip, theApp.m_main_board_temperature, last_main_board_temp, main_board_temp_notify_time, info.GetString());


        //Check whether you want to pop up a traffic usage exceeding prompt
        if (theApp.m_general_data.traffic_tip_enable)
        {
            static __int64 last_today_traffic;
            __int64 traffic_tip_value;
            if (theApp.m_general_data.traffic_tip_unit == 0)
                traffic_tip_value = static_cast<__int64>(theApp.m_general_data.traffic_tip_value) * 1024 * 1024;
            else
                traffic_tip_value = static_cast<__int64>(theApp.m_general_data.traffic_tip_value) * 1024 * 1024 * 1024;

            __int64 today_traffic = theApp.m_today_up_traffic + theApp.m_today_down_traffic;
            if (last_today_traffic < traffic_tip_value && today_traffic >= traffic_tip_value)
            {
                CString info = CCommon::LoadText(IDS_TODAY_TRAFFIC_EXCEED, CCommon::DataSizeToString(today_traffic));
                ShowNotifyTip(CCommon::LoadText(_T("TrafficMonitor "), IDS_NOTIFY, _T("")), info.GetString());
            }
            last_today_traffic = today_traffic;
        }

        CWindowsSettingHelper::CheckWindows10LightTheme();        //Checks every 1 second whether the current system is in white theme

        //Automatically switch taskbar colors based on the current Win10 color mode
        bool light_mode = CWindowsSettingHelper::IsWindows10LightTheme();
        if (theApp.m_last_light_mode != light_mode)
        {
            theApp.m_last_light_mode = light_mode;
            bool restart_taskbar_dlg{ false };
            if (theApp.m_taskbar_data.auto_adapt_light_theme)
            {
                int style_index = CWindowsSettingHelper::IsWindows10LightTheme() ? theApp.m_taskbar_data.light_default_style : theApp.m_taskbar_data.dark_default_style;
                theApp.m_taskbar_default_style.ApplyDefaultStyle(style_index, theApp.m_taskbar_data);
                theApp.SaveConfig();
                restart_taskbar_dlg = true;
            }
            bool is_taskbar_transparent{ theApp.m_taskbar_data.IsTaskbarTransparent()};
            if (!is_taskbar_transparent)
            {
                theApp.m_taskbar_data.SetTaskabrTransparent(false);
                restart_taskbar_dlg = true;
            }
            if (restart_taskbar_dlg && IsTaskbarWndValid())
            {
                m_tBarDlg->ApplyWindowTransparentColor();
                //CloseTaskBarWnd();
                //OpenTaskBarWnd();

                //Write debug log
                if (theApp.m_debug_log)
                {
                    CString log_str;
                    log_str += _T("Detected Windows dark/light theme change.\n");
                    log_str += _T("IsWindows10LightTheme: ");
                    log_str += std::to_wstring(light_mode).c_str();
                    log_str += _T("\n");
                    log_str += _T("auto_adapt_light_theme: ");
                    log_str += std::to_wstring(theApp.m_taskbar_data.auto_adapt_light_theme).c_str();
                    log_str += _T("\n");
                    log_str += _T("is_taskbar_transparent: ");
                    log_str += std::to_wstring(is_taskbar_transparent).c_str();
                    log_str += _T("\n");
                    log_str += _T("taskbar_back_color: ");
                    log_str += std::to_wstring(theApp.m_taskbar_data.back_color).c_str();
                    log_str += _T("\n");
                    log_str += _T("taskbar_transparent_color: ");
                    log_str += std::to_wstring(theApp.m_taskbar_data.transparent_color).c_str();
                    log_str += _T("\n");
                    log_str += _T("taskbar_text_colors: ");
                    for (const auto& item : theApp.m_taskbar_data.text_colors)
                    {
                        log_str += std::to_wstring(item.second.label).c_str();
                        log_str += _T('|');
                        log_str += std::to_wstring(item.second.value).c_str();
                        log_str += _T(", ");
                    }
                    log_str += _T("\n");
                    CCommon::WriteLog(log_str, (theApp.m_config_dir + L".\\debug.log").c_str());
                }
            }

            //Automatically switch the notification area icon according to the current Win10 color mode
            if (theApp.m_cfg_data.m_notify_icon_auto_adapt)
            {
                int notify_icon_selected = theApp.m_cfg_data.m_notify_icon_selected;
                theApp.AutoSelectNotifyIcon();
                if (notify_icon_selected != theApp.m_cfg_data.m_notify_icon_selected)
                {
                    m_ntIcon.hIcon = theApp.m_notify_icons[theApp.m_cfg_data.m_notify_icon_selected];
                    if (theApp.m_general_data.show_notify_icon)
                    {
                        DeleteNotifyIcon();
                        AddNotifyIcon();
                    }
                }
            }

            //Automatically switch skin based on dark/light mode
            if (theApp.m_win_version.IsWindows10OrLater() && theApp.m_cfg_data.skin_auto_adapt)
            {
                int skin_index = CSkinManager::Instance().FindSkinIndex(light_mode ? theApp.m_cfg_data.skin_name_light_mode : theApp.m_cfg_data.skin_name_dark_mode);
                ApplySkin(skin_index);
            }
        }

        //Automatically set the taskbar window background color based on the taskbar color
        if (theApp.m_taskbar_data.auto_set_background_color && theApp.m_win_version.IsWindows8OrLater()
            && IsTaskbarWndValid() && theApp.m_taskbar_data.transparent_color != 0
            && !m_is_foreground_fullscreen && theApp.m_taskbar_data.disable_d2d)
        {
            CRect rect;
            ::GetWindowRect(m_tBarDlg->GetSafeHwnd(), rect);
            int pointx{ rect.left - 1 };
            if (theApp.m_taskbar_data.tbar_wnd_on_left && m_tBarDlg->IsTasksbarOnTopOrBottom())
                pointx = rect.right + 1;
            int pointy = (rect.top + rect.bottom) / 2;
            if (pointx < 0) pointx = 0;
            if (pointx >= m_screen_size.cx) pointx = m_screen_size.cx - 1;
            if (pointy < 0) pointy = 0;
            if (pointy >= m_screen_size.cy) pointy = m_screen_size.cy - 1;
            COLORREF color = ::GetPixel(m_desktop_dc, pointx, pointy);        //Take the color 1 pixel on the left side of the taskbar window as the background color
            if (!CCommon::IsColorSimilar(color, theApp.m_taskbar_data.back_color) && (/*CWindowsSettingHelper::IsWindows10LightTheme() ||*/ color != 0))
            {
                bool is_taskbar_transparent{ theApp.m_taskbar_data.IsTaskbarTransparent()};
                theApp.m_taskbar_data.back_color = color;
                theApp.m_taskbar_data.SetTaskabrTransparent(is_taskbar_transparent);
                if (is_taskbar_transparent)
                    m_tBarDlg->ApplyWindowTransparentColor();
            }
        }

        //When it is detected that both the background color and text color are black, an error log is written.
        static bool erro_log_write{ false };
        if (theApp.m_taskbar_data.back_color == 0 && !theApp.m_taskbar_data.text_colors.empty() && theApp.m_taskbar_data.text_colors.begin()->second.label == 0)
        {
            if (!erro_log_write)
            {
                CString log_str;
                log_str.Format(_T("Both background and text color are black. IsWindows10LightTheme: %d, System uptime: %d/%.2d/%.2d %.2d:%.2d:%.2d"),
                    light_mode, m_start_time.wYear, m_start_time.wMonth, m_start_time.wDay, m_start_time.wHour, m_start_time.wMinute, m_start_time.wSecond);
                CCommon::WriteLog(log_str, theApp.m_log_path.c_str());
                erro_log_write = true;
            }
        }
        else
        {
            erro_log_write = false;
        }

        UpdateNotifyIconTip();


    }

    if (nIDEvent == DELAY_TIMER)
    {
        AutoSelect();
        KillTimer(DELAY_TIMER);
    }

    if (nIDEvent == TASKBAR_TIMER)
    {
        ++m_taskbar_timer_cnt;
        if (m_taskbar_timer_cnt % 5 == 0 && theApp.m_cfg_data.m_show_task_bar_wnd && theApp.m_taskbar_data.show_taskbar_wnd_in_secondary_display)
        {
            static int last_taskbar_num = 0;
            int taskbar_num = CTaskbarHelper::GetSecondaryTaskbarNum();
            //If the number of taskbars on the secondary monitor changes, restart the taskbar window
            if (last_taskbar_num != taskbar_num)
            {
                last_taskbar_num = taskbar_num;
                //Restart the taskbar window after a delay
                KillTimer(RESTART_TASKBAR_TIMER);
                SetTimer(RESTART_TASKBAR_TIMER, 500, [](HWND, UINT, UINT_PTR, DWORD) {
                    theApp.m_pMainWnd->SendMessage(WM_REOPEN_TASKBAR_WND);
                    ::KillTimer(theApp.m_pMainWnd->GetSafeHwnd(), RESTART_TASKBAR_TIMER);
                });
            }
        }

        if (IsTaskbarWndValid())
        {
            //When the main form is hidden at startup, the dpichange message cannot be received, so it needs to be checked manually.
            //Perform a screen DPI check every 100ms*10, and check the operating system version as little as possible
            if (m_taskbar_timer_cnt % 10 == 0 && theApp.m_win_version.IsWindows8Point1OrLater())
            {
                CTaskBarDlg::CheckWindowMonitorDPIAndHandle(*m_tBarDlg, [p_TaskBarDlg = m_tBarDlg](UINT new_dpi_x, UINT new_dpi_y)
                                                            {
                                                                // auto s_dpi = std::to_string(new_dpi_x);
                                                                // s_dpi += '\n';
                                                                // TRACE(s_dpi.c_str());
                                                                //Considering that the taskbar window may not be on the same screen as the main window, the dpi may be different
                                                                //Set DPI and refresh window
                                                                p_TaskBarDlg->SetDPI(new_dpi_x);
                                                                p_TaskBarDlg->SetTextFont();
                                                                p_TaskBarDlg->CalculateWindowSize(); });
            }

            m_tBarDlg->AdjustWindowPos();
            m_tBarDlg->Invalidate(FALSE);
        }
    }

    if (nIDEvent == DELETE_NOTIFY_ICON_TIMER)
    {
        DeleteNotifyIcon();
        KillTimer(DELETE_NOTIFY_ICON_TIMER);
    }

    CDialog::OnTimer(nIDEvent);
}


void CTrafficMonitorDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
    // TODO: Add message handler code and/or call defaults here
    CheckClickedItem(point);
    bool is_plugin_item_clicked = (m_clicked_item.IsPlugin() && m_clicked_item.PluginItem() != nullptr);
    ITMPlugin* plugin{};
    if (is_plugin_item_clicked)
    {
        plugin = theApp.m_plugins.GetPluginByItem(m_clicked_item.PluginItem());
        if (plugin != nullptr && plugin->GetAPIVersion() >= 3)
        {
            if (m_clicked_item.PluginItem()->OnMouseEvent(IPluginItem::MT_RCLICKED, point.x, point.y, (void*)GetSafeHwnd(), 0) != 0)
                return;
        }
    }
    //Set the pop-up menu when clicking the right mouse button
    CMenu* pContextMenu = (is_plugin_item_clicked ? theApp.m_main_menu_plugin.GetSubMenu(0) : theApp.m_main_menu.GetSubMenu(0));
    CPoint point1;  //Define a position used to determine the cursor position
    GetCursorPos(&point1);  //Get the current cursor position so that the menu can follow the cursor
    //Set default menu items
    switch (theApp.m_main_wnd_data.double_click_action)
    {
    case DoubleClickAction::CONNECTION_INFO:
        pContextMenu->SetDefaultItem(ID_NETWORK_INFO);
        break;
        //case DoubleClickAction::HISTORY_TRAFFIC:
        //  pContextMenu->SetDefaultItem(ID_TRAFFIC_HISTORY);
        //  break;
    case DoubleClickAction::SHOW_MORE_INFO:
        pContextMenu->SetDefaultItem(ID_SHOW_CPU_MEMORY);
        break;
    case DoubleClickAction::OPTIONS:
        pContextMenu->SetDefaultItem(ID_OPTIONS);
        break;
        //case DoubleClickAction::CHANGE_SKIN:
        //  pContextMenu->SetDefaultItem(ID_CHANGE_SKIN);
        //  break;
    default:
        pContextMenu->SetDefaultItem(-1);
        break;
    }

    if (plugin != nullptr)
    {
        //Change the displayed text of the plug-in menu in the right-click menu to the plug-in name
        pContextMenu->ModifyMenu(17, MF_BYPOSITION, 17, plugin->GetInfo(ITMPlugin::TMI_NAME));
        //Get plugin icon
        HICON plugin_icon{};
        if (plugin->GetAPIVersion() >= 5)
            plugin_icon = (HICON)plugin->GetPluginIcon();
        //Set plugin icon
        if (plugin_icon != nullptr)
            CMenuIcon::AddIconToMenuItem(pContextMenu->GetSafeHmenu(), 17, TRUE, plugin_icon);
    }
    //Update plugin submenu
    theApp.UpdatePluginMenu(&theApp.m_main_menu_plugin_sub_menu, plugin, 2);

    pContextMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point1.x, point1.y, this); //Show popup menu at specified location

    CDialog::OnRButtonUp(nFlags, point1);
}


void CTrafficMonitorDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
    // TODO: Add message handler code and/or call defaults here
    CheckClickedItem(point);
    bool plugin_item_clicked = false;   //Whether to respond to the left-click event of the plug-in project
    if (m_clicked_item.IsPlugin() && m_clicked_item.PluginItem() != nullptr)      //Whether the clicked item is a plug-in project
    {
        ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(m_clicked_item.PluginItem());
        if (plugin != nullptr && plugin->GetAPIVersion() >= 3)
        {
            if (m_clicked_item.PluginItem()->OnMouseEvent(IPluginItem::MT_LCLICKED, point.x, point.y, (void*)GetSafeHwnd(), 0) != 0)
            {
                plugin_item_clicked = true;
                Invalidate();
            }
        }
    }

    //Allow window dragging by clicking inside window when window position is not locked
    if (!theApp.m_main_wnd_data.m_lock_window_pos && !plugin_item_clicked)
        PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
    CDialog::OnLButtonDown(nFlags, point);
}


void CTrafficMonitorDlg::OnNetworkInfo()
{
    // TODO: Add command handler code here
    //The "Connection Details" dialog box pops up
    CNetworkInfoDlg aDlg(m_connections, m_pIfTable->table, m_connection_selected);
    ////Pass the CNetworkInfoDlg class the number of bytes sent and received since startup
    //aDlg.m_in_bytes = m_pIfTable->table[m_connections[m_connection_selected].index].dwInOctets - m_connections[m_connection_selected].in_bytes;
    //aDlg.m_out_bytes = m_pIfTable->table[m_connections[m_connection_selected].index].dwOutOctets - m_connections[m_connection_selected].out_bytes;
    aDlg.m_start_time = m_start_time;
    aDlg.DoModal();
    //SetAlwaysOnTop(); //Since the cancellation of the window on top is set in the "Connection Details" dialog box, after the dialog box is closed, the window on top is reset.
    if (m_tBarDlg != nullptr)
        m_tBarDlg->m_tool_tips.SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);  //Reset the prompt message of the taskbar window to the top
}


void CTrafficMonitorDlg::OnAlwaysOnTop()
{
    // TODO: Add command handler code here
    theApp.m_main_wnd_data.m_always_on_top = !theApp.m_main_wnd_data.m_always_on_top;
    SetAlwaysOnTop();
    theApp.SaveConfig();
}


void CTrafficMonitorDlg::OnTransparency100()
{
    // TODO: Add command handler code here
    theApp.m_cfg_data.m_transparency = 100;
    SetTransparency();
    theApp.SaveConfig();
}


void CTrafficMonitorDlg::OnTransparency80()
{
    // TODO: Add command handler code here
    theApp.m_cfg_data.m_transparency = 80;
    SetTransparency();
    theApp.SaveConfig();
}


void CTrafficMonitorDlg::OnTransparency60()
{
    // TODO: Add command handler code here
    theApp.m_cfg_data.m_transparency = 60;
    SetTransparency();
    theApp.SaveConfig();
}


void CTrafficMonitorDlg::OnTransparency40()
{
    // TODO: Add command handler code here
    theApp.m_cfg_data.m_transparency = 40;
    SetTransparency();
    theApp.SaveConfig();
}


void CTrafficMonitorDlg::OnClose()
{
    // TODO: Add message handler code and/or call defaults here
    theApp.m_cannot_save_config_warning = true;
    theApp.m_cannot_save_global_config_warning = true;
    theApp.SaveConfig();    //Save settings to ini file before exiting
    theApp.SaveGlobalConfig();
    SaveHistoryTrafficFull();  // Use full save when exiting to ensure all data is saved
    BackupHistoryTrafficFile();

    if (IsTaskbarWndValid())
        m_tBarDlg->OnCancel();

    //Make sure to close all windows before exiting
    for (const auto& item : CBaseDialog::AllUniqueHandels())
    {
        ::SendMessage(item.second, WM_COMMAND, IDCANCEL, 0);
    }

    CDialog::OnClose();
}


BOOL CTrafficMonitorDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    // TODO: Add specialized code here and/or call the base class
    UINT uMsg = LOWORD(wParam);
    if (uMsg == ID_SELECT_ALL_CONNECTION)
    {
        theApp.m_cfg_data.m_select_all = true;
        theApp.m_cfg_data.m_auto_select = false;
        m_connection_change_flag = true;
    }
    //Processing when an item in the "Select Network Connection" submenu is selected
    if (uMsg == ID_SELETE_CONNECTION)   //The "Autoselect" menu item is selected
    {
        AutoSelect();
        theApp.m_cfg_data.m_auto_select = true;
        theApp.m_cfg_data.m_select_all = false;
        theApp.SaveConfig();
        m_connection_change_flag = true;
    }
    if (uMsg > ID_SELECT_ALL_CONNECTION && uMsg <= ID_SELECT_ALL_CONNECTION + m_connections.size()) //Selected a network connection
    {
        m_connection_selected = uMsg - ID_SELECT_ALL_CONNECTION - 1;
        theApp.m_cfg_data.m_connection_name = GetConnection(m_connection_selected).description_2;
        m_connection_name_preferd = theApp.m_cfg_data.m_connection_name;
        theApp.m_cfg_data.m_auto_select = false;
        theApp.m_cfg_data.m_select_all = false;
        theApp.SaveConfig();
        m_connection_change_flag = true;
    }
#ifdef DEBUG
    if (uMsg == ID_CMD_TEST)
    {
        CTest::TestCommand();
        //ShowNotifyTip(CCommon::LoadText(_T("TrafficMonitor "), IDS_NOTIFY, _T("")), _T("Test Notification"));

    }
#endif // DEBUG
    //Plugin command selected
    if (uMsg >= ID_PLUGIN_COMMAND_START && uMsg <= ID_PLUGIN_COMMAND_MAX)
    {
        int index = uMsg - ID_PLUGIN_COMMAND_START;
        if (m_clicked_item.IsPlugin() && m_clicked_item.PluginItem() != nullptr)
        {
            ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(m_clicked_item.PluginItem());
            if (plugin != nullptr && plugin->GetAPIVersion() >= 5)
            {
                plugin->OnPluginCommand(index, (void*)GetSafeHwnd(), nullptr);
            }
        }
    }


    return CDialog::OnCommand(wParam, lParam);
    }


void CTrafficMonitorDlg::OnInitMenu(CMenu* pMenu)
{
    CDialog::OnInitMenu(pMenu);

    // TODO: add message handler code here
    m_menu_popuped = true;

    pMenu->CheckMenuItem(ID_ALWAYS_ON_TOP, MF_BYCOMMAND | (theApp.m_main_wnd_data.m_always_on_top ? MF_CHECKED : MF_UNCHECKED));
    pMenu->CheckMenuItem(ID_LOCK_WINDOW_POS, MF_BYCOMMAND | (theApp.m_main_wnd_data.m_lock_window_pos ? MF_CHECKED : MF_UNCHECKED));
    pMenu->CheckMenuItem(ID_SHOW_CPU_MEMORY, MF_BYCOMMAND | (theApp.m_cfg_data.m_show_more_info ? MF_CHECKED : MF_UNCHECKED));
    pMenu->CheckMenuItem(ID_MOUSE_PENETRATE, MF_BYCOMMAND | (theApp.m_main_wnd_data.m_mouse_penetrate ? MF_CHECKED : MF_UNCHECKED));
    pMenu->CheckMenuItem(ID_SHOW_TASK_BAR_WND, MF_BYCOMMAND | (theApp.m_cfg_data.m_show_task_bar_wnd ? MF_CHECKED : MF_UNCHECKED));
    pMenu->DeleteMenu(ID_SHOW_MAIN_WND, MF_BYCOMMAND);
    pMenu->CheckMenuItem(ID_ALOW_OUT_OF_BORDER, MF_BYCOMMAND | (theApp.m_main_wnd_data.m_alow_out_of_border ? MF_CHECKED : MF_UNCHECKED));

    //Set the selection status of each option in the "Select Connection" submenu item
    SetConnectionMenuState(theApp.m_main_menu.GetSubMenu(0)->GetSubMenu(0));
    SetConnectionMenuState(theApp.m_main_menu_plugin.GetSubMenu(0)->GetSubMenu(0));

    //Set the selection status of each option under the "Window Opacity" submenu
    switch (theApp.m_cfg_data.m_transparency)
    {
    case 100: pMenu->CheckMenuRadioItem(ID_TRANSPARENCY_100, ID_TRANSPARENCY_40, ID_TRANSPARENCY_100, MF_BYCOMMAND | MF_CHECKED); break;
    case 80: pMenu->CheckMenuRadioItem(ID_TRANSPARENCY_100, ID_TRANSPARENCY_40, ID_TRANSPARENCY_80, MF_BYCOMMAND | MF_CHECKED); break;
    case 60: pMenu->CheckMenuRadioItem(ID_TRANSPARENCY_100, ID_TRANSPARENCY_40, ID_TRANSPARENCY_60, MF_BYCOMMAND | MF_CHECKED); break;
    case 40: pMenu->CheckMenuRadioItem(ID_TRANSPARENCY_100, ID_TRANSPARENCY_40, ID_TRANSPARENCY_40, MF_BYCOMMAND | MF_CHECKED); break;
    default: break;
    }

    if (theApp.IsForceShowNotifyIcon())    //If the taskbar window is not displayed and the main window is hidden or mouse penetration is set, disable the "Show notification area icon" menu item
        pMenu->EnableMenuItem(ID_SHOW_NOTIFY_ICON, MF_BYCOMMAND | MF_GRAYED);
    else
        pMenu->EnableMenuItem(ID_SHOW_NOTIFY_ICON, MF_BYCOMMAND | MF_ENABLED);

    pMenu->EnableMenuItem(ID_SELECT_ALL_CONNECTION, MF_BYCOMMAND | (theApp.m_general_data.show_all_interface ? MF_GRAYED : MF_ENABLED));

    pMenu->EnableMenuItem(ID_CHECK_UPDATE, MF_BYCOMMAND | (theApp.IsCheckingForUpdate() ? MF_GRAYED : MF_ENABLED));

    //Set the check status of the plug-in command
    ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(m_clicked_item.PluginItem());
    if (plugin != nullptr && plugin->GetAPIVersion() >= 5)
    {
        for (int i = ID_PLUGIN_COMMAND_START; i <= ID_PLUGIN_COMMAND_MAX; i++)
        {
            bool checked = (plugin->IsCommandChecked(i) != 0);
            pMenu->CheckMenuItem(i, MF_BYCOMMAND | (checked ? MF_CHECKED : MF_UNCHECKED));
        }
    }

    //pMenu->SetDefaultItem(ID_NETWORK_INFO);
}


BOOL CTrafficMonitorDlg::PreTranslateMessage(MSG* pMsg)
{
    // TODO: Add specialized code here and/or call the base class
    //To shield, press Enter and ESC to exit.
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE) return TRUE;
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) return TRUE;

    if (theApp.m_main_wnd_data.show_tool_tip && m_tool_tips.GetSafeHwnd())
    {
        m_tool_tips.RelayEvent(pMsg);
    }

    if (pMsg->message == WM_KEYDOWN)
    {
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x80);
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000);
        bool alt = (GetKeyState(VK_MENU) & 0x8000);
        ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(m_clicked_item.PluginItem());
        if (plugin != nullptr && plugin->GetAPIVersion() >= 4)
        {
            if (m_clicked_item.PluginItem()->OnKeboardEvent(pMsg->wParam, ctrl, shift, alt, (void*)GetSafeHwnd(), IPluginItem::KF_TASKBAR_WND) != 0)
                return TRUE;
        }
    }

    return CDialog::PreTranslateMessage(pMsg);
}


void CTrafficMonitorDlg::OnLockWindowPos()
{
    // TODO: Add command handler code here
    theApp.m_main_wnd_data.m_lock_window_pos = !theApp.m_main_wnd_data.m_lock_window_pos;
    theApp.SaveConfig();
}


void CTrafficMonitorDlg::OnMove(int x, int y)
{
    CDialog::OnMove(x, y);

    // TODO: add message handler code here

    if (!m_first_start)
    {
        theApp.m_cfg_data.m_position_x = x;
        theApp.m_cfg_data.m_position_y = y;
    }

    ////Make sure the window does not extend beyond the screen
    //CheckWindowPos();
}



afx_msg LRESULT CTrafficMonitorDlg::OnNotifyIcon(WPARAM wParam, LPARAM lParam)
{
    bool dialog_exist{ false };
    HWND handle{};
    if (lParam == WM_LBUTTONDOWN || lParam == WM_RBUTTONUP || lParam == WM_LBUTTONDBLCLK)
    {
        for (const auto& item : CBaseDialog::AllUniqueHandels())
        {
            if (IsWindow(item.second))
            {
                dialog_exist = true;
                handle = item.second;
                break;
            }
        }
    }

    if (lParam == WM_LBUTTONDOWN)
    {
        if (!theApp.m_cfg_data.m_hide_main_window)
        {
            if (dialog_exist)       //When there is an open dialog box, click the notification area icon to set focus to the dialog box.
            {
                ::SetForegroundWindow(handle);
            }
            else        //When there is no open dialog box, the main window is displayed.
            {
                ShowWindow(SW_RESTORE);
                theApp.m_cfg_data.m_hide_main_window = false;
                SetForegroundWindow();
                SetAlwaysOnTop();
                CheckWindowPos();
                theApp.SaveConfig();
            }
        }
    }
    if (lParam == WM_RBUTTONUP)
    {
        if (dialog_exist)       //When there is an open dialog box, click the notification area icon to set focus to the dialog box.
        {
            ::SetForegroundWindow(handle);
        }
        else
        {
            //Right-click in the notification area to pop up the right-click menu
            if (IsTaskbarWndValid())        //If a taskbar window is displayed, set focus to the taskbar window after right-clicking the notification area icon
                m_tBarDlg->SetForegroundWindow();
            else                //Otherwise set focus to the main window
                SetForegroundWindow();
            CPoint point1;  //Define a position used to determine the cursor position
            GetCursorPos(&point1);  //Get the current cursor position so that the menu can follow the cursor
            theApp.m_main_menu.GetSubMenu(0)->SetDefaultItem(-1);       //Settings has no default menu item
            theApp.m_main_menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point1.x, point1.y, this); //Show popup menu at specified location

            CheckWindowPos();
        }
    }
    if (lParam == WM_LBUTTONDBLCLK)
    {
        if (dialog_exist)       //When there is an open dialog box, click the notification area icon to set focus to the dialog box.
        {
            ::SetForegroundWindow(handle);
        }
        else        //When there is no open dialog box, the main window is displayed.
        {
            ShowWindow(SW_RESTORE);
            theApp.m_cfg_data.m_hide_main_window = false;
            SetForegroundWindow();
            SetAlwaysOnTop();
            CheckWindowPos();
            theApp.SaveConfig();
        }
    }
    return 0;
}


void CTrafficMonitorDlg::OnShowNotifyIcon()
{
    // TODO: Add command handler code here
    if (theApp.m_general_data.show_notify_icon)
    {
        DeleteNotifyIcon();
        theApp.m_general_data.show_notify_icon = false;
    }
    else
    {
        AddNotifyIcon();
        theApp.m_general_data.show_notify_icon = true;
    }
    theApp.SaveConfig();
}


void CTrafficMonitorDlg::OnDestroy()
{
    CDialog::OnDestroy();

    //Remove notification bar icon when program exits
    ::Shell_NotifyIcon(NIM_DELETE, &m_ntIcon);

    // Stop monitoring thread
    ExitMonitorThread();
}


void CTrafficMonitorDlg::OnShowCpuMemory()
{
    // TODO: Add command handler code here
    CRect rect;
    GetWindowRect(rect);
    theApp.m_cfg_data.m_show_more_info = !theApp.m_cfg_data.m_show_more_info;
    if (theApp.m_cfg_data.m_show_more_info)
    {
        rect.right = rect.left + m_skin.GetLayoutInfo().layout_l.width;
        rect.bottom = rect.top + m_skin.GetLayoutInfo().layout_l.width;
        MoveWindow(rect);
        CheckWindowPos();
    }
    else
    {
        rect.right = rect.left + m_skin.GetLayoutInfo().layout_s.width;
        rect.bottom = rect.top + m_skin.GetLayoutInfo().layout_s.height;
        MoveWindow(rect);
        CheckWindowPos();
    }
    LoadBackGroundImage();
    SetItemPosition();
    Invalidate(FALSE);
    theApp.SaveConfig();
}


//Processing when the taskbar window switches to display CPU and memory utilization
void CTrafficMonitorDlg::OnShowCpuMemory2()
{
    // TODO: Add command handler code here
    if (IsTaskbarWndValid())
    {
        bool show_cpu_memory = (theApp.m_taskbar_data.display_item.Contains(TDI_CPU) || theApp.m_taskbar_data.display_item.Contains(TDI_MEMORY));
        if (show_cpu_memory)
        {
            theApp.m_taskbar_data.display_item.Remove(TDI_CPU);
            theApp.m_taskbar_data.display_item.Remove(TDI_MEMORY);
        }
        else
        {
            theApp.m_taskbar_data.display_item.Add(TDI_CPU);
            theApp.m_taskbar_data.display_item.Add(TDI_MEMORY);
        }
        //theApp.m_cfg_data.m_tbar_show_cpu_memory = !theApp.m_cfg_data.m_tbar_show_cpu_memory;
        //When switching to display CPU and memory utilization, delete the taskbar window and then redisplay it.
        //CloseTaskBarWnd();
        //OpenTaskBarWnd();
        m_tBarDlg->WidthChanged();
    }
}


void CTrafficMonitorDlg::OnMousePenetrate()
{
    // TODO: Add command handler code here
    theApp.m_main_wnd_data.m_mouse_penetrate = !theApp.m_main_wnd_data.m_mouse_penetrate;
    SetMousePenetrate();
    if (!theApp.m_general_data.show_notify_icon && theApp.IsForceShowNotifyIcon())   //When the mouse penetrates, if the notification icon is not displayed, display it, otherwise the right-click menu cannot be called out.
    {
        //Add notification bar icon
        AddNotifyIcon();
        theApp.m_general_data.show_notify_icon = true;
    }

    //After setting mouse penetration, a message pops up prompting the user how to turn off mouse penetration.
    if (theApp.m_main_wnd_data.m_mouse_penetrate && theApp.m_show_mouse_panetrate_tip)
    {
        if (MessageBox(CCommon::LoadText(IDS_MOUSE_PENETRATE_TIP_INFO), NULL, MB_ICONINFORMATION | MB_OKCANCEL) == IDCANCEL)        //No more prompts after clicking "Cancel"
        {
            theApp.m_show_mouse_panetrate_tip = false;
        }
    }

    theApp.SaveConfig();
}


void CTrafficMonitorDlg::OnShowTaskBarWnd()
{
    // TODO: Add command handler code here
    if (m_tBarDlg != nullptr)
    {
        CloseTaskBarWnd();
    }
    if (!theApp.m_cfg_data.m_show_task_bar_wnd)
    {
        theApp.m_cfg_data.m_show_task_bar_wnd = true;
        OpenTaskBarWnd();
    }
    else
    {
        theApp.m_cfg_data.m_show_task_bar_wnd = false;
        //After closing the taskbar window, if the notification area icon is not displayed and the main window is not displayed or mouse penetration is set, the notification area icon will be displayed.
        if (!theApp.m_general_data.show_notify_icon && theApp.IsForceShowNotifyIcon())
        {
            AddNotifyIcon();
            theApp.m_general_data.show_notify_icon = true;
        }
    }
    theApp.SaveConfig();
}


void CTrafficMonitorDlg::OnAppAbout()
{
    // TODO: Add command handler code here
    //Pop up the "About" dialog box
    CAboutDlg aDlg;
    aDlg.DoModal();
}


//This message is triggered when the resource manager is restarted
LRESULT CTrafficMonitorDlg::OnTaskBarCreated(WPARAM wParam, LPARAM lParam)
{
    if (m_tBarDlg != nullptr)
    {
        CloseTaskBarWnd();
        if (theApp.m_general_data.show_notify_icon)
        {
            //Re-add notification bar icon
            ::Shell_NotifyIcon(NIM_ADD, &m_ntIcon);
        }
        OpenTaskBarWnd();
    }
    else
    {
        if (theApp.m_general_data.show_notify_icon)
            ::Shell_NotifyIcon(NIM_ADD, &m_ntIcon);
    }
    return LRESULT();
}


void CTrafficMonitorDlg::OnShowMainWnd()
{
    // TODO: Add command handler code here
    if (!theApp.m_cfg_data.m_hide_main_window)
    {
        ShowWindow(SW_HIDE);
        theApp.m_cfg_data.m_hide_main_window = true;
        //After hiding the main window, if the notification bar icon is not shown, show it
        if (!theApp.m_general_data.show_notify_icon && theApp.IsForceShowNotifyIcon())
        {
            AddNotifyIcon();
            theApp.m_general_data.show_notify_icon = true;
        }
    }
    else
    {
        ShowWindow(SW_RESTORE);
        theApp.m_cfg_data.m_hide_main_window = false;
    }
    theApp.SaveConfig();
}


void CTrafficMonitorDlg::OnChangeSkin()
{
    // TODO: Add command handler code here
    CSkinDlg skinDlg;
    //Initialize the data of the CSkinDlg object
    skinDlg.m_skin_selected = m_skin_selected;
    if (skinDlg.DoModal() == IDOK)
    {
        ApplySkin(skinDlg.m_skin_selected);
    }
}



void CTrafficMonitorDlg::OnTrafficHistory()
{
    // TODO: Add command handler code here
    CHistoryTrafficDlg historyDlg(m_history_traffic.GetTraffics());
    historyDlg.DoModal();
}


void CTrafficMonitorDlg::OnMouseMove(UINT nFlags, CPoint point)
{
    // TODO: Add message handler code and/or call defaults here

    CDialog::OnMouseMove(nFlags, point);
}


void CTrafficMonitorDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    // TODO: Add message handler code and/or call defaults here
    CheckClickedItem(point);
    if (m_clicked_item.IsPlugin() && m_clicked_item.PluginItem() != nullptr)
    {
        ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(m_clicked_item.PluginItem());
        if (plugin != nullptr && plugin->GetAPIVersion() >= 3)
        {
            if (m_clicked_item.PluginItem()->OnMouseEvent(IPluginItem::MT_DBCLICKED, point.x, point.y, (void*)GetSafeHwnd(), 0) != 0)
                return;
        }
    }

    switch (theApp.m_main_wnd_data.double_click_action)
    {
    case DoubleClickAction::CONNECTION_INFO:
        OnNetworkInfo();            //After double-clicking, the "Connection Details" dialog box will pop up.
        break;
    case DoubleClickAction::HISTORY_TRAFFIC:
        OnTrafficHistory();         //After double-clicking, the "Historical Traffic Statistics" dialog box will pop up.
        break;
    case DoubleClickAction::SHOW_MORE_INFO:
        OnShowCpuMemory();          //Toggle display of CPU and memory utilization
        break;
    case DoubleClickAction::OPTIONS:
        OnOptions();                //After double-clicking, the "Option Settings" dialog box will pop up.
        break;
    case DoubleClickAction::TASK_MANAGER:
        ShellExecuteW(NULL, _T("open"), (theApp.m_system_dir + L"\\Taskmgr.exe").c_str(), NULL, NULL, SW_NORMAL);       //Open task manager
        break;
    case DoubleClickAction::SEPCIFIC_APP:
        ShellExecuteW(NULL, _T("open"), (theApp.m_main_wnd_data.double_click_exe).c_str(), NULL, NULL, SW_NORMAL);  //Open the specified program, the default task manager
        break;
    case DoubleClickAction::CHANGE_SKIN:
        OnChangeSkin();             //After double-clicking, the "Change Skin" dialog box pops up.
        break;
    default:
        break;
    }
    CDialog::OnLButtonDblClk(nFlags, point);
}


void CTrafficMonitorDlg::OnOptions()
{
    _OnOptions(0, this);
}


//Open the "Options" dialog box via the right-click menu of the taskbar window
void CTrafficMonitorDlg::OnOptions2()
{
    CWnd* pParent = this;
    if (IsTaskbarWndValid())
        pParent = m_tBarDlg;
    _OnOptions(1, pParent);
}


afx_msg LRESULT CTrafficMonitorDlg::OnExitmenuloop(WPARAM wParam, LPARAM lParam)
{
    m_menu_popuped = false;
    return 0;
}


void CTrafficMonitorDlg::OnChangeNotifyIcon()
{
    // TODO: Add command handler code here
    CIconSelectDlg dlg(theApp.m_cfg_data.m_notify_icon_selected);
    dlg.SetAutoAdaptNotifyIcon(theApp.m_cfg_data.m_notify_icon_auto_adapt);
    if (dlg.DoModal() == IDOK)
    {
        theApp.m_cfg_data.m_notify_icon_selected = dlg.GetIconSelected();
        theApp.m_cfg_data.m_notify_icon_auto_adapt = dlg.AutoAdaptNotifyIcon();
        m_ntIcon.hIcon = theApp.m_notify_icons[theApp.m_cfg_data.m_notify_icon_selected];
        if (theApp.m_cfg_data.m_notify_icon_auto_adapt)
            theApp.AutoSelectNotifyIcon();
        if (theApp.m_general_data.show_notify_icon)
        {
            DeleteNotifyIcon();
            AddNotifyIcon();
        }
        theApp.SaveConfig();
    }
}


void CTrafficMonitorDlg::OnAlowOutOfBorder()
{
    // TODO: Add command handler code here
    theApp.m_main_wnd_data.m_alow_out_of_border = !theApp.m_main_wnd_data.m_alow_out_of_border;
    CheckWindowPos();
}


void CTrafficMonitorDlg::OnCheckUpdate()
{
    // TODO: Add command handler code here
    theApp.CheckUpdateInThread(true);
}


afx_msg LRESULT CTrafficMonitorDlg::OnTaskbarMenuPopedUp(WPARAM wParam, LPARAM lParam)
{
    //Set the selection status of each option in the "Select Connection" submenu item
    SetConnectionMenuState(theApp.m_taskbar_menu.GetSubMenu(0)->GetSubMenu(0));
    SetConnectionMenuState(theApp.m_taskbar_menu_plugin.GetSubMenu(0)->GetSubMenu(0));
    return 0;
}


//Handling when the taskbar window switches to display network speed
void CTrafficMonitorDlg::OnShowNetSpeed()
{
    // TODO: Add command handler code here
    if (IsTaskbarWndValid())
    {
        bool show_net_speed = (theApp.m_taskbar_data.display_item.Contains(TDI_UP) || theApp.m_taskbar_data.display_item.Contains(TDI_DOWN));
        if (show_net_speed)
        {
            theApp.m_taskbar_data.display_item.Remove(TDI_UP);
            theApp.m_taskbar_data.display_item.Remove(TDI_DOWN);
        }
        else
        {
            theApp.m_taskbar_data.display_item.Add(TDI_UP);
            theApp.m_taskbar_data.display_item.Add(TDI_DOWN);
        }
        //CloseTaskBarWnd();
        //OpenTaskBarWnd();
        m_tBarDlg->WidthChanged();
    }
}


BOOL CTrafficMonitorDlg::OnQueryEndSession()
{
    if (!CDialog::OnQueryEndSession())
        return FALSE;

    // TODO: Add dedicated query end session code here
    theApp.SaveConfig();
    theApp.SaveGlobalConfig();
    SaveHistoryTrafficFull();  // Use a full save when the system is shut down to ensure all data is saved
    BackupHistoryTrafficFile();

    if (theApp.m_debug_log)
    {
        CCommon::WriteLog(_T("TrafficMonitor process terminated, settings saved."), (theApp.m_config_dir + L".\\debug.log").c_str());
    }

    return TRUE;
}


void CTrafficMonitorDlg::OnPaint()
{
    CPaintDC dc(this); // device context for painting
                       // TODO: add message handler code here
                       // CDialog::OnPaint() is not called for drawing messages
    m_skin.DrawInfo(&dc, theApp.m_cfg_data.m_show_more_info);
}


afx_msg LRESULT CTrafficMonitorDlg::OnDpichanged(WPARAM wParam, LPARAM lParam)
{
    static int dpi;
    static CTrafficMonitorDlg* pThis;
    dpi = LOWORD(wParam);
    pThis = this;

    //Since multiple DPI change messages will be triggered in a short period of time when hovering and dragging to displays with different DPIs, the message is delayed for a period of time before processing.
    KillTimer(DPI_CHANGE_TIMER);
    SetTimer(DPI_CHANGE_TIMER, 500, [](HWND, UINT, UINT_PTR, DWORD) {
        //Get the DPI based on the position of the main window
        CRect rect;
        pThis->GetWindowRect(rect);
        UINT dpi_x, dpi_y;
        if (theApp.DPIFromRect(rect, &dpi_x, &dpi_y))   //If the acquisition is successful, the dpi obtained based on the position of the main window will be used.
            dpi = dpi_x;
        TRACE("Dpi changed: %d\n", dpi);

        theApp.SetDPI(dpi);
        //Use the original behavior when the system version is smaller than Windows 8.1
        if (pThis->IsTaskbarWndValid() && !theApp.m_win_version.IsWindows8Point1OrLater())
        {
            //Reassign DPI to taskbar windows
            pThis->m_tBarDlg->SetDPI(dpi);
            //Reset the taskbar window font according to the new DPI
            pThis->m_tBarDlg->SetTextFont();
        }

        pThis->LoadSkinLayout();   //Get layout data based on the currently selected skin
        pThis->SetItemPosition();  //Initialize window position
        pThis->LoadBackGroundImage();
        pThis->SetTextFont();      //Reload font
        pThis->Invalidate(FALSE);  //Redraw interface

        pThis->KillTimer(DPI_CHANGE_TIMER);
    });

    return 0;
}


afx_msg LRESULT CTrafficMonitorDlg::OnTaskbarWndClosed(WPARAM wParam, LPARAM lParam)
{
    theApp.m_cfg_data.m_show_task_bar_wnd = false;
    //After closing the taskbar window, if the notification area icon is not displayed and the main window is not displayed or mouse penetration is set, the notification area icon will be displayed.
    if (!theApp.m_general_data.show_notify_icon && theApp.IsForceShowNotifyIcon())
    {
        AddNotifyIcon();
        theApp.m_general_data.show_notify_icon = true;
    }
    return 0;
}



afx_msg LRESULT CTrafficMonitorDlg::OnMonitorInfoUpdated(WPARAM wParam, LPARAM lParam)
{
    Invalidate(FALSE);      //Refresh window information

    //Update mouse tips
    if (theApp.m_main_wnd_data.show_tool_tip && m_tool_tips.GetSafeHwnd() != NULL)
    {
        CString tip_info;
        tip_info = GetMouseTipsInfo();
        m_tool_tips.UpdateTipText(tip_info, this);
    }
    //Update taskbar window mouse tips
    if (IsTaskbarWndValid())
        m_tBarDlg->UpdateToolTips();
    return 0;
}


LRESULT CTrafficMonitorDlg::OnDisplaychange(WPARAM wParam, LPARAM lParam)
{
    GetScreenSize();
    CheckWindowPos(true);
    return 0;
}


void CTrafficMonitorDlg::OnExitSizeMove()
{
    // TODO: Add message handler code and/or call defaults here
    CheckWindowPos();

    CDialog::OnExitSizeMove();
}


void CTrafficMonitorDlg::OnPluginManage()
{
    // TODO: Add command handler code here
    CPluginManagerDlg dlg;
    dlg.DoModal();
}

LRESULT CTrafficMonitorDlg::OnReopenTaksbarWnd(WPARAM wParam, LPARAM lParam)
{
    CloseTaskBarWnd();
    OpenTaskBarWnd();
    return 0;
}


void CTrafficMonitorDlg::OnOpenTaskManager()
{
    ShellExecuteW(NULL, _T("open"), (theApp.m_system_dir + L"\\Taskmgr.exe").c_str(), NULL, NULL, SW_NORMAL);       //Open task manager
}


afx_msg LRESULT CTrafficMonitorDlg::OnSettingsApplied(WPARAM wParam, LPARAM lParam)
{
    COptionsDlg* pOptionsDlg = (COptionsDlg*)wParam;
    if (pOptionsDlg != nullptr)
    {
        ApplySettings(*pOptionsDlg);
    }
    return 0;
}


void CTrafficMonitorDlg::OnDisplaySettings()
{
    // TODO: Add command handler code here
    CSetItemOrderDlg dlg;
    dlg.SetItemOrder(theApp.m_taskbar_data.item_order.GetItemOrderConst());
    dlg.SetDisplayItem(theApp.m_taskbar_data.display_item);
    dlg.SetPluginDisplayItem(theApp.m_taskbar_data.plugin_display_item);
    if (dlg.DoModal() == IDOK)
    {
        theApp.m_taskbar_data.item_order.SetOrder(dlg.GetItemOrder());
        theApp.m_taskbar_data.display_item = dlg.GetDisplayItem();
        theApp.m_taskbar_data.plugin_display_item = dlg.GetPluginDisplayItem();
        //CloseTaskBarWnd();
        //OpenTaskBarWnd();
        if (IsTaskbarWndValid())
            m_tBarDlg->WidthChanged();
    }
}


void CTrafficMonitorDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
    // TODO: Add message handler code and/or call defaults here
    //CheckClickedItem(point);
    //if (m_clicked_item.IsPlugin() && m_clicked_item.PluginItem() != nullptr)
    //{
    //    ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(m_clicked_item.PluginItem());
    //    if (plugin != nullptr && plugin->GetAPIVersion() >= 3)
    //    {
    //        if (m_clicked_item.PluginItem()->OnMouseEvent(IPluginItem::MT_LCLICKED, point.x, point.y, (void*)GetSafeHwnd(), 0) != 0)
    //            return;
    //    }
    //}

    CDialog::OnLButtonUp(nFlags, point);
}


void CTrafficMonitorDlg::OnRefreshConnectionList()
{
    IniConnection();

}


afx_msg LRESULT CTrafficMonitorDlg::OnTabletQuerysystemgesturestatus(WPARAM wParam, LPARAM lParam)
{
    return 0;
}


void CTrafficMonitorDlg::OnPluginOptions()
{
    if (m_clicked_item.IsPlugin())
    {
        //Find the corresponding plug-in
        ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(m_clicked_item.PluginItem());
        if (plugin != nullptr)
        {
            //Show plugin option settings
            auto rtn = plugin->ShowOptionsDialog(GetSafeHwnd());
            if (rtn == ITMPlugin::OR_OPTION_NOT_PROVIDED)
                MessageBox(CCommon::LoadText(IDS_PLUGIN_NO_OPTIONS_INFO), nullptr, MB_ICONINFORMATION | MB_OK);
        }
    }
}


void CTrafficMonitorDlg::OnPluginDetail()
{
    if (m_clicked_item.IsPlugin())
    {
        //Find the corresponding plug-in
        ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(m_clicked_item.PluginItem());
        if (plugin != nullptr)
        {
            int index = theApp.m_plugins.GetPluginIndex(plugin);
            CPluginInfoDlg dlg(index);
            dlg.DoModal();
        }
    }
}


void CTrafficMonitorDlg::OnPluginOptionsTaksbar()
{
    //Determine whether the clicked item in the taskbar window is a plug-in item
    if (IsTaskbarWndValid() && m_tBarDlg->GetClickedItem().IsPlugin())
    {
        //Find the corresponding plug-in
        ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(m_tBarDlg->GetClickedItem().PluginItem());
        if (plugin != nullptr)
        {
            //Show plugin option settings
            auto rtn = plugin->ShowOptionsDialog(m_tBarDlg->GetSafeHwnd());
            if (rtn == ITMPlugin::OR_OPTION_CHANGED)    //The option settings have been changed, reopen the taskbar window
            {
                //CloseTaskBarWnd();
                //OpenTaskBarWnd();
                m_tBarDlg->WidthChanged();
            }
            if (rtn == ITMPlugin::OR_OPTION_NOT_PROVIDED)
                MessageBox(CCommon::LoadText(IDS_PLUGIN_NO_OPTIONS_INFO), nullptr, MB_ICONINFORMATION | MB_OK);
        }
    }
}


void CTrafficMonitorDlg::OnPluginDetailTaksbar()
{
    if (IsTaskbarWndValid() && m_tBarDlg->GetClickedItem().IsPlugin())
    {
        //Find the corresponding plug-in
        ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(m_tBarDlg->GetClickedItem().PluginItem());
        if (plugin != nullptr)
        {
            int index = theApp.m_plugins.GetPluginIndex(plugin);
            CPluginInfoDlg dlg(index);
            dlg.DoModal();
        }
    }
}


UINT CTrafficMonitorDlg::OnPowerBroadcast(UINT nPowerEvent, LPARAM nEventData)
{
    // System resumes from hibernation
    if (nPowerEvent == PBT_APMRESUMESUSPEND)
    {
        //Reinitialize the network connection after a delay
        KillTimer(INIT_CONNECT_TIMER);
        static CTrafficMonitorDlg* pThis = this;
        static int check_times = 0;
        SetTimer(INIT_CONNECT_TIMER, 10000, [](HWND, UINT, UINT_PTR, DWORD) {
            pThis->IniConnection();
            check_times++;

            //write log
            CString info = CCommon::LoadTextFormat(IDS_RESTORE_FROM_SLEEP_LOG, {pThis->m_restart_cnt });
            CCommon::WriteLog(info, theApp.m_log_path.c_str());

            //If the connection is empty, the timer continues to run and the connection is reinitialized at regular intervals.
            if (pThis->m_connections.size() == 0)
            {
                //More than 20 times, end timer
                if (check_times >= 20)
                    pThis->KillTimer(INIT_CONNECT_TIMER);
            }
            //Successfully obtain the connection and end the timer
            else
            {
                pThis->KillTimer(INIT_CONNECT_TIMER);
                check_times = 0;
            }
        });
    }
    return CDialog::OnPowerBroadcast(nPowerEvent, nEventData);
}


void CTrafficMonitorDlg::OnColorizationColorChanged(DWORD dwColorizationColor, BOOL bOpacity)
{
    // This feature requires Windows Vista or later.
    // _WIN32_WINNT symbol must be >= 0x0600.
    // TODO: Add message handler code and/or call defaults here

    static DWORD last_color;
    if (last_color != dwColorizationColor)
    {
        last_color = dwColorizationColor;
        BYTE red = (dwColorizationColor >> 16) & 0xFF;
        BYTE green = (dwColorizationColor >> 8) & 0xFF;
        BYTE blue = dwColorizationColor & 0xFF;
        COLORREF theme_color = RGB(red, green, blue);
        theApp.SetThemeColor(theme_color);
    }
    CDialog::OnColorizationColorChanged(dwColorizationColor, bOpacity);
}
