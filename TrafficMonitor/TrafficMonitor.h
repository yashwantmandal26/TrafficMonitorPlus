
// TrafficMonitor.h : Main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
#error "Include stdafx.h before this file for PCH"
#endif

#include "resource.h"       // main symbol
#include "Common.h"
#include "WinVersionHelper.h"
#include "SimpleXML.h"
#include "TaskbarDefaultStyle.h"
#include <map>
#include "OpenHardwareMonitor/OpenHardwareMonitorApi.h"
#include "PluginManager.h"
#include "Nullable.hpp"
#include "TaskBarDlgDrawCommon.h"
#include "DllFunctions.h"
#include "StrTable.h"
#include "PluginUpdateHelper.h"
#include "PluginInterface.h"

// CTrafficMonitorApp:
// For an implementation of this class, see TrafficMonitor.cpp
//


class CTrafficMonitorApp : public CWinApp, public ITrafficMonitor
{
public:
    //Various paths
    static CTrafficMonitorApp* self;
    wstring m_module_dir;       //Directory of program exe file
    wstring m_appdata_dir;
    wstring m_module_path;      //The path to the program exe file
    wstring m_module_path_reg;  //The path to the exe file used as the startup entry to be written to the registry (if there are spaces in the path, add quotes)
    wstring m_config_path;
    wstring m_history_traffic_path;
    wstring m_log_path;
    wstring m_skin_path;
    wstring m_system_dir;
    wstring m_config_dir;

    //The following data is defined as public members in the App class so that it can be accessed in both the main dialog box and the taskbar window.
    unsigned __int64 m_in_speed{};      //Download speed
    unsigned __int64 m_out_speed{};     //upload speed
    int m_cpu_usage{ -1 };      //CPU utilization
    int m_memory_usage{ -1 };   //memory utilization
    int m_used_memory{};    //Available physical memory (in KB)
    int m_total_memory{};   //Total physical memory (unit: KB)
    float m_cpu_temperature{ -1 };  //CPU temperature
    float m_cpu_freq{ -1 };  //CPU frequency
    float m_gpu_temperature{ -1 };  //Graphics card temperature
    float m_hdd_temperature{ -1 };  //Hard drive temperature
    float m_main_board_temperature{ -1 };    //Motherboard temperature
    int m_gpu_usage{ -1 };      //Graphics card utilization
    bool IsDgpuActive();
    DWORD m_last_gpu_check_tick{ 0 };
    bool m_cached_has_igpu{ false };
    bool m_cached_has_dgpu{ false };
    int m_hdd_usage{ -1 };      //Hard drive utilization

    unsigned __int64 m_today_up_traffic{};  //Upload traffic used today
    unsigned __int64 m_today_down_traffic{};    //Download traffic used today

    bool m_cannot_save_config_warning{ true };  //Indicates whether a prompt box will pop up when settings cannot be saved
    bool m_cannot_save_global_config_warning{ true };   //Indicates whether a prompt box will pop up when settings cannot be saved

    bool m_module_dir_writable{ true };         //Indicates whether the directory where the program is located is writable

    //option setting data
    MainWndSettingData m_main_wnd_data;
    TaskBarSettingData m_taskbar_data;
    GeneralSettingData m_general_data;
    //Other setting data
    MainConfigData m_cfg_data;
    int m_notify_interval;      //The time interval between pop-up notification messages
    bool m_debug_log{};
    bool m_taksbar_transparent_color_enable{};
    bool m_last_light_mode{};
    bool m_show_mouse_panetrate_tip{};  //Whether to display the prompt message when "mouse penetration" is turned on.
    bool m_show_dot_net_notinstalled_tip{};

    //bool m_is_windows10_fall_creator;
    CWinVersionHelper m_win_version;        //Current version of Windows

    HICON m_notify_icons[MAX_NOTIFY_ICON];

    CTaskbarDefaultStyle m_taskbar_default_style;
    CPluginManager m_plugins;
    CDllFunctions m_dll_functions;
    CStrTable m_str_table;
    CPluginUpdateHelper m_plugin_update;

    CMenu m_main_menu;          //Main window right-click menu
    CMenu m_main_menu_plugin;   //Right-click the right-click menu of the plug-in area of the main window
    CMenu m_main_menu_plugin_sub_menu;
    CMenu m_taskbar_menu;       //Taskbar window right-click menu
    CMenu m_taskbar_menu_plugin;    //Right-click the context menu of the plug-in area of the taskbar window
    CMenu m_taskbar_menu_plugin_sub_menu;

#ifndef WITHOUT_TEMPERATURE
    //Pointer to the OpenHardwareMonitor interface
    std::shared_ptr<OpenHardwareMonitorApi::IOpenHardwareMonitor> m_pMonitor{};
#endif // !WITHOUT_TEMPERATURE

    CCriticalSection m_minitor_lib_critical;        //Critical section object used to access OpenHardwareMonitor for thread synchronization
    //CCriticalSection m_lftable_critical; //Critical section object used to access LfTable2 for thread synchronization
    CLazyConstructable<class CTaskBarDlgDrawCommonSupport> m_d2d_taskbar_draw_common_support{}; // Global dependencies automatically initialized when using D2D rendering

public:
    CTrafficMonitorApp();

    void LoadLanguageConfig();
    void LoadConfig();
    void SaveConfig();
    void LoadPluginDisabledSettings();

    void LoadGlobalConfig();
    void SaveGlobalConfig();

    int DPI(int pixel);
    void DPI(CRect& rect);
    void DPIFromWindow(CWnd* pWnd);
    int GetDpi() const { return m_dpi; }
    void SetDPI(int dpi) { m_dpi = dpi; }

    void CheckUpdate(bool message);     //Check for updates. If message is true, a prompt message will pop up during the check.
    void CheckUpdateInThread(bool message); //Check for updates in background thread
    //Check update thread function at startup
    static UINT CheckUpdateThreadFunc(LPVOID lpParam);
    static UINT InitOpenHardwareMonitorLibThreadFunc(LPVOID lpParam);

    bool SetAutoRun(bool auto_run, bool task_scheduler);
    bool GetAutoRun(wstring* auto_run_path, bool task_scheduler);        //Determine whether it will run automatically at boot. If so, write the path to auto-run at boot into auto_run_path.

    bool SetAutoRunByRegistry(bool auto_run);       //Auto-start at boot through registry
    bool SetAutoRunByTaskScheduler(bool auto_run);  //Auto-start at boot via task schedule

    //Get system information text
    CString GetSystemInfoString();

    void InitMenuResourse();

    //Get an icon resource. If the resource has not been loaded, it will be loaded automatically.
    //Since the CTrafficMonitorApp::DPI function is used in this function, this function must be called after CTrafficMonitorApp::DPIFromWindow
    HICON GetMenuIcon(UINT id);

    void AutoSelectNotifyIcon();

    bool IsCheckingForUpdate() const { return m_checking_update; }      //Checking for updates

    void InitOpenHardwareLibInThread();     //Start a background thread to initialize OpenHardwareMonitor
    void UpdateOpenHardwareMonitorEnableState();    //Update enabled/disabled status of hardware monitoring

    //void UpdateTaskbarWndMenu(); //Update the taskbar window right-click menu
    bool IsForceShowNotifyIcon();       //Do you need to force the notification area icon to be displayed?

    std::wstring GetPlauginTooltipInfo() const;
    bool IsTaksbarItemDisplayed(CommonDisplayItem item) const;

    void SendSettingsToPlugin();    //Send current options settings to all plugins

    //Update plugin submenu
    //plugin_cmd_start_index: The starting position of the plug-in command in the menu
    static void UpdatePluginMenu(CMenu* pMenu, ITMPlugin* plugin, int plugin_cmd_start_index);

    void CheckWindows11Taskbar();
    bool IsWindows11Taskbar() const { return m_is_windows11_taskbar; }

    bool DPIFromRect(const RECT& rect, UINT* out_dpi_x, UINT* out_dpi_y);

    virtual unsigned int GetThemeColor() const override;
    void SetThemeColor(COLORREF color);

private:
    //int m_no_multistart_warning_time{}; //Used to set how long after booting the warning prompt "There is already a program running" will not pop up.
    bool m_no_multistart_warning{};         //If false, the "There is already a program running" warning will never pop up.
    bool m_exit_when_start_by_restart_manager{ true };      //Exit the program if it is restarted by Windows Restart Manager
    int m_dpi{ 96 };

    bool m_checking_update{ false };        //Checking for updates

    std::map<UINT, HICON> m_menu_icons;      //Menu icon resource. key is the ID of the icon resource, vlaue is the handle of the icon

    ULONG_PTR m_gdiplusToken{};

    bool m_is_windows11_taskbar{ false };  //Is it the taskbar of Windows 11?
    COLORREF m_theme_color{};

// rewrite
public:
    virtual BOOL InitInstance();

    // realize

    DECLARE_MESSAGE_MAP()
    afx_msg void OnHelp();
    afx_msg void OnFrequentyAskedQuestions();
    afx_msg void OnUpdateLog();
    virtual int ExitInstance();

    // Inherited through ITrafficMonitor
private:
    virtual int GetAPIVersion() override;
    virtual const wchar_t* GetVersion() override;
    double GetMonitorValue(MonitorItem item) override;
    virtual const wchar_t* GetMonitorValueString(MonitorItem item, int is_main_window = false) override;
    virtual const wchar_t* GetStringRes(const wchar_t* key, const wchar_t* section) override;
public:
    void ShowNotifyMessage(const wchar_t* strMsg) override;
    unsigned short GetLanguageId() const override;
    const wchar_t* GetPluginConfigDir() const override;
    int GetDPI(DPIType type) const override;
    virtual void* GetMainWindowHwnd() override;
    virtual void* GetTaskbarWindowHwnd() override;
};

extern CTrafficMonitorApp theApp;
