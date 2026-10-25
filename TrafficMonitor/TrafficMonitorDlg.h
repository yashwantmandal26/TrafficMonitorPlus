
// TrafficMonitorDlg.h: header file
//

#pragma once
#pragma comment (lib, "iphlpapi.lib")

#include "NetworkInfoDlg.h"
#include "afxwin.h"
#include "StaticEx.h"
#include "Common.h"
#include "TaskBarDlg.h"
#include "SkinDlg.h"
#include "HistoryTrafficDlg.h"
#include "OptionsDlg.h"
#include "PictureStatic.h"
#include "IconSelectDlg.h"
#include "DrawCommon.h"
#include "IniHelper.h"
#include "LinkStatic.h"
#include "AdapterCommon.h"
#include "AboutDlg.h"
#include "PdhHardwareQuery/CPUUsage.h"
#include "PdhHardwareQuery/CpuFreq.h"
#include "PdhHardwareQuery/GpuUsage.h"
#include "PdhHardwareQuery/DiskUsage.h"
#include "PdhHardwareQuery/CpuTemperature.h"
#include "HistoryTrafficFile.h"

// CTrafficMonitorDlg dialog box
class CTrafficMonitorDlg : public CDialog
{
    // structure
public:
    CTrafficMonitorDlg(CWnd* pParent = NULL);   // Standard constructor
    ~CTrafficMonitorDlg();
    CTaskBarDlg* GetTaskbarWindow() const;

    static CTrafficMonitorDlg* Instance();

    // Dialog data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_TRAFFICMONITOR_DIALOG };
#endif

    const CSkinFile& GetCurSkin() const { return m_skin; }

    CPdhDiskUsage& GetPdhDiskUsageHelper() { return m_disk_usage_helper; }
    bool IsGetDiskUsageByPdh() const { return m_get_disk_usage_by_pdh; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support


// realize
protected:
    HICON m_hIcon;
    NOTIFYICONDATA m_ntIcon;    //notification area icon
    CTaskBarDlg* m_tBarDlg{};     //Taskbar window pointer

    vector<NetWorkConection> m_connections; //Save all obtained network connections to be displayed in the "Select Network Card" menu item
    MIB_IFTABLE* m_pIfTable;
    DWORD m_dwSize{};	//The size of m_pIfTable
    int m_connection_selected{ 0 }; //The sequence number of the connection to display traffic
    unsigned __int64 m_in_bytes{};        //Current number of bytes received
    unsigned __int64 m_out_bytes{};   //The current number of bytes sent
    unsigned __int64 m_last_in_bytes{}; //Number of bytes last received
    unsigned __int64 m_last_out_bytes{};    //Number of bytes last sent

    CCPUUsage m_cpu_usage_helper;
    CPdhCpuFreq m_cpu_freq_helper;
    CPdhGPUUsage m_gpu_usage_helper;
    CPdhDiskUsage m_disk_usage_helper;
    CPdhCpuTemperature m_cpu_temp_helper;

    bool m_get_disk_usage_by_pdh{};

    bool m_first_start{ true };     //It is true initially and is set to false after the timer is started for the first time.

    // https://www.jianshu.com/p/9d4b68cdbd99
    struct Monitors
    {
        std::vector<MONITORINFO> monitorinfos;

        static BOOL CALLBACK MonitorEnum(HMONITOR hMon, HDC hdc, LPRECT lprcMonitor, LPARAM pData)
        {
            MONITORINFO iMonitor;
            iMonitor.cbSize = sizeof(MONITORINFO);
            GetMonitorInfo(hMon, &iMonitor);

            Monitors* pThis = reinterpret_cast<Monitors*>(pData);
            pThis->monitorinfos.push_back(iMonitor);
            return TRUE;
        }

        Monitors()
        {
            EnumDisplayMonitors(0, 0, MonitorEnum, (LPARAM)this);
        }
    };

    //CRect m_screen_rect; //The range of the screen (excluding the taskbar)
    vector<CRect> m_screen_rects;       //Range of all screens (excluding taskbar)
    vector<CRect> m_last_screen_rects;       //The range of all previous screens (not including the taskbar)
    CSize m_screen_size;        //Screen size (including taskbar)
    CSkinFile m_skin;
    CommonDisplayItem m_clicked_item;           //Item clicked by mouse

    int m_restart_cnt{ -1 };    //Number of reinitializations
    unsigned int m_timer_cnt{};     //The number of times the timer has fired (number of seconds since the program started)
    unsigned int m_taskbar_timer_cnt{0}; //Number of timer fires for TaskBarDlg (number of seconds since program started)
    unsigned int m_monitor_time_cnt{};
    int m_zero_speed_cnt{}; //If the network speed cannot be detected, this variable will increase automatically.
    int m_insert_to_taskbar_cnt{};  //Used to count the number of attempts to embed the taskbar
    int m_cannot_insert_to_task_bar_warning{ true };   //Indicates whether a prompt box will pop up when the taskbar cannot be embedded.

    static unsigned int m_WM_TASKBARCREATED;    //Taskbar restart message

    int m_skin_selected{};      //Selected skin number

    SYSTEMTIME m_start_time;    //The time when the program started
    CHistoryTrafficFile m_history_traffic{ theApp.m_history_traffic_path }; //Store historical traffic

    CToolTipCtrl m_tool_tips;

    bool m_connection_change_flag{ false };     //If the IniConnection() function has been executed, this flag will be set to true
    bool m_is_foreground_fullscreen{ false };   //Indicates whether the foreground window is being displayed globally
    bool m_menu_popuped{ false };               //Indicates whether a menu is currently pop-up

    HDC m_desktop_dc;

    string m_connection_name_preferd{ theApp.m_cfg_data.m_connection_name };          //Save the network connection name manually selected by the user

    void DoMonitorAcquisition();    //Get monitoring information once
    static UINT MonitorThreadCallback(LPVOID dwUser);   //Thread function to obtain monitoring information
    bool m_monitor_data_required{ false };          //The monitoring data flag needs to be obtained in the thread. When monitoring data needs to be obtained, it is set to true. When monitoring data is obtained once, it is set to false.
    bool m_is_thread_exit{ false }; //Thread exit flag
    CEvent m_threadExitEvent;       //Used to notify the main thread that the worker thread has exited
public:
    void ExitMonitorThread();       //Stop monitoring thread

protected:
    CString GetMouseTipsInfo();     //Get mouse prompt information
    void SetTransparency();         //Set window transparency according to the value of m_transparency
    void SetTransparency(int transparency);
public:
    void SetAlwaysOnTop();          //Set the window to the top according to the value of m_always_on_top
protected:
    void SetMousePenetrate();       //Set whether mouse penetration is based on the value of m_mouse_penetrate
    POINT CalculateWindowMoveOffset(CRect rect, bool screen_changed);  //Calculate the position required to move the window to the screen area when it is outside the screen area
    void CheckWindowPos(bool screen_changed = false);          //Test the position of the window. If the position of the window is outside the screen, move the window so that it is all within the screen and return to the new position.
    void GetScreenSize();           //Get the size of the screen

    void AutoSelect();
    //void UpdateConnections();
    //Automatically select connection
    void IniConnection();   //Initialize connection

    MIB_IFROW GetConnectIfTable(int connection_index);    //Gets the MIB_IFROW object of the currently selected network connection. connection_index is the index in m_connections
    NetWorkConection GetConnection(int connection_index); //Gets the NetWorkConection object of the currently selected network connection. connection_index is the index in m_connections

    void IniConnectionMenu(CMenu* pMenu);   //Initialize the "Select a Network Connection" menu
    void IniTaskBarConnectionMenu();        //Initialize the taskbar window's "Select a network connection" menu
    void SetConnectionMenuState(CMenu* pMenu);      //Set the selected item in the "Select a network connection" menu

    void CloseTaskBarWnd(); //Close taskbar window
    void OpenTaskBarWnd();  //Open taskbar window

    void AddNotifyIcon();       //Add notification area icon
    void DeleteNotifyIcon();
public:
    void ShowNotifyTip(const wchar_t* title, const wchar_t* message);       //Show notification area prompt
protected:
    void UpdateNotifyIconTip();     //Updated mouse tips for notification area icons

    void SaveHistoryTraffic();        // Incremental save, only update the first row and today's record
    void SaveHistoryTrafficFull();    // Complete save, used to ensure that all data is saved when the program exits
    void LoadHistoryTraffic();
    void BackupHistoryTrafficFile();

    void _OnOptions(int tab, CWnd* pParent);   //Processing of opening the "Options" dialog box, tab: the tab to be switched when opening

    void ApplySettings(COptionsDlg& optionsDlg);

    void SetItemPosition();     //Set the position of the 4 items displayed
    bool LoadSkinLayout();      //Get layout data from current skin

    void LoadBackGroundImage();
    void SetTextFont();

public:
    bool IsTaskbarWndValid() const;
protected:

    void TaskbarShowHideItem(DisplayItem type);

    //Determine which display item area a point is in and save it to m_clicked_item
    void CheckClickedItem(CPoint point);

    //apply a skin
    void ApplySkin(int skin_index);

public:
    //void ApplySettings();
    bool IsTemperatureNeeded() const;       //Determine whether temperature information needs to be displayed

protected:
    // Generated message mapping function
    virtual BOOL OnInitDialog();
    //  afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()
public:
    //  afx_msg LRESULT OnNcHitTest(CPoint point);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    //  afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnNetworkInfo();
    afx_msg void OnAlwaysOnTop();
    afx_msg void OnTransparency100();
    afx_msg void OnTransparency80();
    afx_msg void OnTransparency60();
    afx_msg void OnTransparency40();
    afx_msg void OnClose();
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    afx_msg void OnInitMenu(CMenu* pMenu);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnLockWindowPos();
    afx_msg void OnMove(int x, int y);
protected:
    afx_msg LRESULT OnNotifyIcon(WPARAM wParam, LPARAM lParam);
public:
    afx_msg void OnShowNotifyIcon();
    afx_msg void OnDestroy();
    afx_msg void OnShowCpuMemory();
    afx_msg void OnMousePenetrate();
    //afx_msg void OnTextColor();
    afx_msg void OnShowTaskBarWnd();
    afx_msg void OnAppAbout();
    afx_msg void OnShowCpuMemory2();
    afx_msg void OnShowMainWnd();
    afx_msg void OnChangeSkin();
    afx_msg LRESULT OnTaskBarCreated(WPARAM wParam, LPARAM lParam);
    //afx_msg void OnSetFont();
    afx_msg void OnTrafficHistory();
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnOptions();
    afx_msg void OnOptions2();
protected:
    afx_msg LRESULT OnExitmenuloop(WPARAM wParam, LPARAM lParam);
public:
    afx_msg void OnChangeNotifyIcon();
    afx_msg void OnAlowOutOfBorder();
    afx_msg void OnCheckUpdate();
protected:
    afx_msg LRESULT OnTaskbarMenuPopedUp(WPARAM wParam, LPARAM lParam);
public:
    afx_msg void OnShowNetSpeed();
    afx_msg BOOL OnQueryEndSession();
    afx_msg void OnPaint();
protected:
    afx_msg LRESULT OnDpichanged(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnTaskbarWndClosed(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnMonitorInfoUpdated(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnDisplaychange(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnReopenTaksbarWnd(WPARAM wParam, LPARAM lParam);
public:
    afx_msg void OnExitSizeMove();
    afx_msg void OnPluginManage();
    afx_msg void OnOpenTaskManager();
protected:
    afx_msg LRESULT OnSettingsApplied(WPARAM wParam, LPARAM lParam);
public:
    afx_msg void OnDisplaySettings();
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnRefreshConnectionList();
protected:
    afx_msg LRESULT OnTabletQuerysystemgesturestatus(WPARAM wParam, LPARAM lParam);
public:
    afx_msg void OnPluginOptions();
    afx_msg void OnPluginDetail();
    afx_msg void OnPluginOptionsTaksbar();
    afx_msg void OnPluginDetailTaksbar();
    afx_msg UINT OnPowerBroadcast(UINT nPowerEvent, LPARAM nEventData);
    afx_msg void OnColorizationColorChanged(DWORD dwColorizationColor, BOOL bOpacity);
};
