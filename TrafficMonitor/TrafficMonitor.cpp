
// TrafficMonitor.cpp: Defines the class behavior of the application.
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "TrafficMonitorDlg.h"
#include "crashtool.h"
#include "UpdateHelper.h"
#include "Test.h"
#include "WIC.h"
#include "auto_start_helper.h"
#include "AppAlreadyRuningDlg.h"
#include "WindowsSettingHelper.h"
#include "SkinManager.h"
#include "SettingsHelper.h"
#ifndef DISABLE_WINDOWS_WEB_EXPERIENCE_DETECTOR
#include "winrt/base.h"
#endif
#include <gdiplus.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTrafficMonitorApp

BEGIN_MESSAGE_MAP(CTrafficMonitorApp, CWinApp)
    //ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
    ON_COMMAND(ID_HELP, &CTrafficMonitorApp::OnHelp)
    ON_COMMAND(ID_FREQUENTY_ASKED_QUESTIONS, &CTrafficMonitorApp::OnFrequentyAskedQuestions)
    ON_COMMAND(ID_UPDATE_LOG, &CTrafficMonitorApp::OnUpdateLog)
END_MESSAGE_MAP()


CTrafficMonitorApp* CTrafficMonitorApp::self = NULL;


// CTrafficMonitorApp construct
CTrafficMonitorApp::CTrafficMonitorApp()
{
    self = this;
    // Support restart manager
    //m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

    // TODO: Add construction code here,
    // Place all important initialization in InitInstance
    CRASHREPORT::StartCrashReport();
#ifndef DISABLE_WINDOWS_WEB_EXPERIENCE_DETECTOR
    if (m_win_version.IsWindows11OrLater())
        winrt::init_apartment();
#endif

    CheckWindows11Taskbar();
    m_theme_color = CCommon::GetWindowsThemeColor();
}

void CTrafficMonitorApp::LoadLanguageConfig()
{
    CIniHelper ini{ m_config_path };
    wstring lang_str = ini.GetString(_T("general"), _T("language"), L"");
    if (lang_str.empty() || lang_str == L"0")
    {
        m_general_data.language.bcp_47 = L"en-US";
        m_general_data.language.display_name = L"English";
        m_general_data.language.default_font_name = L"Segoe UI";
        m_general_data.language.language_id = LocaleNameToLCID(L"en-US", 0);
    }
    else
    {
        m_general_data.language.fromConfigString(lang_str);
    }
}

void CTrafficMonitorApp::LoadConfig()
{
    CSettingsHelper ini;

    //General settings
    m_general_data.check_update_when_start = ini.GetBool(_T("general"), _T("check_update_when_start"), true);
    //m_general_data.allow_skin_cover_font = ini.GetBool(_T("general"), _T("allow_skin_cover_font"), true);
    //m_general_data.allow_skin_cover_text = ini.GetBool(_T("general"), _T("allow_skin_cover_text"), true);
    m_general_data.show_all_interface = ini.GetBool(L"general", L"show_all_interface", false);
    bool is_chinese_language{ m_str_table.IsSimplifiedChinese() };     //Whether the current language is Simplified Chinese
    m_general_data.update_source = ini.GetInt(L"general", L"update_source", 0);   //If the current language is Simplified, the default update source is Gitee, otherwise it is GitHub
    m_general_data.cpu_usage_acquire_method = static_cast<GeneralSettingData::CpuUsageAcquireMethod>(ini.GetInt(L"general", L"cpu_usage_acquire_method", GeneralSettingData::CA_PDH));
    m_general_data.monitor_time_span = ini.GetInt(L"general", L"monitor_time_span", 1000);
    if (m_general_data.monitor_time_span < MONITOR_TIME_SPAN_MIN || m_general_data.monitor_time_span > MONITOR_TIME_SPAN_MAX)
        m_general_data.monitor_time_span = 1000;
    m_general_data.hard_disk_name = ini.GetString(L"general", L"hard_disk_name", L"");
    m_general_data.cpu_core_name = ini.GetString(L"general", L"cpu_core_name", L"Core Average");
    m_general_data.hardware_monitor_item = ini.GetInt(L"general", L"hardware_monitor_item", 0);
    std::vector<std::wstring> connections_hide;
    ini.GetStringList(L"general", L"connections_hide", connections_hide, std::vector<std::wstring>{});
    m_general_data.connections_hide.FromVector(connections_hide);

    //Windows 10 color mode settings
    bool is_windows10_light_theme = CWindowsSettingHelper::IsWindows10LightTheme();
    if (is_windows10_light_theme)
        CCommon::SetColorMode(ColorMode::Light);
    else
        CCommon::SetColorMode(ColorMode::Default);

    //Main window settings
    m_cfg_data.m_transparency = ini.GetInt(_T("config"), _T("transparency"), 80);
    m_main_wnd_data.m_always_on_top = ini.GetBool(_T("config"), _T("always_on_top"), true);
    m_main_wnd_data.m_lock_window_pos = ini.GetBool(_T("config"), _T("lock_window_pos"), false);
    m_general_data.show_notify_icon = ini.GetBool(_T("config"), _T("show_notify_icon"), true);
    m_cfg_data.m_show_more_info = ini.GetBool(_T("config"), _T("show_cpu_memory"), false);
    m_main_wnd_data.m_mouse_penetrate = ini.GetBool(_T("config"), _T("mouse_penetrate"), false);
    m_cfg_data.m_show_task_bar_wnd = ini.GetBool(_T("config"), _T("show_task_bar_wnd"), false);
    m_cfg_data.m_position_x = ini.GetInt(_T("config"), _T("position_x"), -1);
    m_cfg_data.m_position_y = ini.GetInt(_T("config"), _T("position_y"), -1);
    m_cfg_data.m_auto_select = ini.GetBool(_T("connection"), _T("auto_select"), true);
    m_cfg_data.m_select_all = ini.GetBool(_T("connection"), _T("select_all"), false);
    m_cfg_data.m_hide_main_window = ini.GetBool(_T("config"), _T("hide_main_window"), false);
    m_cfg_data.m_connection_name = CCommon::UnicodeToStr(ini.GetString(L"connection", L"connection_name", L"").c_str());
    m_cfg_data.m_skin_name = ini.GetString(_T("config"), _T("skin_selected"), _T(""));
    if (m_cfg_data.m_skin_name.substr(0, 8) == L".\\skins\\")       //If the skin name read has ".\\skins\\" in front of it, delete it. (To maintain compatibility with the previous version)
        m_cfg_data.m_skin_name = m_cfg_data.m_skin_name.substr(7);
    CSkinManager::SkinNameNormalize(m_cfg_data.m_skin_name);

    m_cfg_data.skin_auto_adapt = ini.GetBool(L"skins", L"skin_auto_adapt", false);
    m_cfg_data.skin_name_dark_mode = ini.GetString(L"skins", L"skin_name_dark_mode", L"");
    m_cfg_data.skin_name_light_mode = ini.GetString(L"skins", L"skin_name_light_mode", L"");

    m_cfg_data.m_notify_icon_selected = ini.GetInt(_T("config"), _T("notify_icon_selected"), (m_win_version.IsWindows7() || m_win_version.IsWindows8Or8point1() ? 2 : m_cfg_data.m_dft_notify_icon));       //Win7/8/8.1 uses blue notification area icons by default, because the white icons will not be visible clearly after hiding the notification area icons. Other systems use white icons by default.
    m_cfg_data.m_notify_icon_auto_adapt = ini.GetBool(_T("config"), _T("notify_icon_auto_adapt"), true);
    if (m_cfg_data.m_notify_icon_auto_adapt)
        AutoSelectNotifyIcon();
    m_main_wnd_data.swap_up_down = ini.GetBool(_T("config"), _T("swap_up_down"), false);
    m_main_wnd_data.hide_main_wnd_when_fullscreen = ini.GetBool(_T("config"), _T("hide_main_wnd_when_fullscreen"), true);

    //Since the main window's text color, font, and display text settings are no longer read from the configuration file, these settings are saved in separate settings for each skin, so they are only initialized to fixed values here.
    m_main_wnd_data.text_colors[TDI_UP] = RGB(255, 255, 255);
    m_main_wnd_data.specify_each_item_color = false;
    m_main_wnd_data.font.name = m_str_table.GetLanguageInfo().default_font_name.c_str();
    m_main_wnd_data.font.size = 10;
    m_main_wnd_data.disp_str.Get(TDI_UP) = CCommon::LoadText(IDS_UPLOAD_DISP, L": ");
    m_main_wnd_data.disp_str.Get(TDI_DOWN) = CCommon::LoadText(IDS_DOWNLOAD_DISP, L": ");
    m_main_wnd_data.disp_str.Get(TDI_CPU) = L"CPU: ";
    m_main_wnd_data.disp_str.Get(TDI_MEMORY) = CCommon::LoadText(IDS_MEMORY_DISP, _T(": "));

    m_main_wnd_data.speed_short_mode = ini.GetBool(_T("config"), _T("speed_short_mode"), false);
    m_main_wnd_data.separate_value_unit_with_space = ini.GetBool(_T("config"), _T("separate_value_unit_with_space"), true);
    m_main_wnd_data.show_tool_tip = ini.GetBool(_T("config"), _T("show_tool_tip"), true);
    m_main_wnd_data.memory_display = static_cast<MemoryDisplay>(ini.GetInt(L"config", L"memory_display", static_cast<int>(MemoryDisplay::USAGE_PERCENTAGE)));
    m_main_wnd_data.unit_byte = ini.GetBool(_T("config"), _T("unit_byte"), true);
    m_main_wnd_data.speed_unit = static_cast<SpeedUnit>(ini.GetInt(_T("config"), _T("speed_unit"), 0));
    m_main_wnd_data.hide_unit = ini.GetBool(_T("config"), _T("hide_unit"), false);
    m_main_wnd_data.hide_percent = ini.GetBool(_T("config"), _T("hide_percent"), false);
    m_main_wnd_data.double_click_action = static_cast<DoubleClickAction>(ini.GetInt(_T("config"), _T("double_click_action"), 0));
    m_main_wnd_data.double_click_exe = ini.GetString(L"config", L"double_click_exe", (theApp.m_system_dir + L"\\Taskmgr.exe").c_str());

    m_main_wnd_data.m_alow_out_of_border = ini.GetBool(_T("config"), _T("alow_out_of_border"), false);

    m_general_data.traffic_tip_enable = ini.GetBool(L"notify_tip", L"traffic_tip_enable", false);
    m_general_data.traffic_tip_value = ini.GetInt(L"notify_tip", L"traffic_tip_value", 200);
    m_general_data.traffic_tip_unit = ini.GetInt(L"notify_tip", L"traffic_tip_unit", 0);
    m_general_data.memory_usage_tip.enable = ini.GetBool(L"notify_tip", L"memory_usage_tip_enable", false);
    m_general_data.memory_usage_tip.tip_value = ini.GetInt(L"notify_tip", L"memory_tip_value", 80);
    m_general_data.cpu_temp_tip.enable = ini.GetBool(L"notify_tip", L"cpu_temperature_tip_enable", false);
    m_general_data.cpu_temp_tip.tip_value = ini.GetInt(L"notify_tip", L"cpu_temperature_tip_value", 80);
    m_general_data.gpu_temp_tip.enable = ini.GetBool(L"notify_tip", L"gpu_temperature_tip_enable", false);
    m_general_data.gpu_temp_tip.tip_value = ini.GetInt(L"notify_tip", L"gpu_temperature_tip_value", 80);
    m_general_data.hdd_temp_tip.enable = ini.GetBool(L"notify_tip", L"hdd_temperature_tip_enable", false);
    m_general_data.hdd_temp_tip.tip_value = ini.GetInt(L"notify_tip", L"hdd_temperature_tip_value", 80);
    m_general_data.mainboard_temp_tip.enable = ini.GetBool(L"notify_tip", L"mainboard_temperature_tip_enable", false);
    m_general_data.mainboard_temp_tip.tip_value = ini.GetInt(L"notify_tip", L"mainboard_temperature_tip_value", 80);

    //Taskbar window settings
    m_taskbar_data.back_color = ini.GetInt(_T("task_bar"), _T("task_bar_back_color"), m_taskbar_data.dft_back_color);
    m_taskbar_data.transparent_color = ini.GetInt(_T("task_bar"), _T("transparent_color"), m_taskbar_data.dft_transparent_color);
    if (m_taskbar_data.IsTaskbarTransparent()) //If the taskbar background is transparent, you need to convert the color
    {
        CCommon::TransparentColorConvert(m_taskbar_data.back_color);
        CCommon::TransparentColorConvert(m_taskbar_data.transparent_color);
    }
    m_taskbar_data.status_bar_color = ini.GetInt(_T("task_bar"), _T("status_bar_color"), m_taskbar_data.dft_status_bar_color);
    //m_taskbar_data.text_color = GetPrivateProfileInt(_T("task_bar"), _T("task_bar_text_color"), 0x00ffffffU, m_config_path.c_str());
    ini.LoadTaskbarWndColors(_T("task_bar"), _T("task_bar_text_color"), m_taskbar_data.text_colors, m_taskbar_data.dft_text_colors);
    m_taskbar_data.specify_each_item_color = ini.GetBool(L"task_bar", L"specify_each_item_color", false);
    //m_cfg_data.m_tbar_show_cpu_memory = ini.GetBool(_T("task_bar"), _T("task_bar_show_cpu_memory"), false);
    m_taskbar_data.display_item.FromInt(ini.GetInt(L"task_bar", L"tbar_display_item", DisplayItemSet{ TDI_UP, TDI_DOWN }.ToInt()));
    m_taskbar_data.show_taskbar_wnd_in_secondary_display = ini.GetBool(L"task_bar", L"show_taskbar_wnd_in_secondary_display", false);
    m_taskbar_data.secondary_display_index = ini.GetInt(L"task_bar", L"secondary_display_index", 0);

    //The version without temperature monitoring does not display temperature monitoring related items.
#ifdef WITHOUT_TEMPERATURE
    m_taskbar_data.display_item.Remove(TDI_CPU_TEMP);
    m_taskbar_data.display_item.Remove(TDI_GPU_TEMP);
    m_taskbar_data.display_item.Remove(TDI_HDD_TEMP);
    m_taskbar_data.display_item.Remove(TDI_MAIN_BOARD_TEMP);
#endif

    //If a certain hardware monitoring is turned off in the option settings, the corresponding temperature monitoring related items will not be displayed.
    if (!m_general_data.IsHardwareEnable(HI_CPU))
        m_taskbar_data.display_item.Remove(TDI_CPU_TEMP);
    if (!m_general_data.IsHardwareEnable(HI_GPU))
    {
        m_taskbar_data.display_item.Remove(TDI_GPU_TEMP);
    }
    if (!m_general_data.IsHardwareEnable(HI_HDD))
    {
        m_taskbar_data.display_item.Remove(TDI_HDD_TEMP);
    }
    if (!m_general_data.IsHardwareEnable(HI_MBD))
        m_taskbar_data.display_item.Remove(TDI_MAIN_BOARD_TEMP);

    //m_taskbar_data.swap_up_down = ini.GetBool(_T("task_bar"), _T("task_bar_swap_up_down"), false);

    if (m_taskbar_data.back_color == 0 && !m_taskbar_data.text_colors.empty() && m_taskbar_data.text_colors.begin()->second.label == 0)     //If the read background color and text color are both 0 (black), set the text color and background color to the default color
    {
        m_taskbar_data.back_color = m_taskbar_data.dft_back_color;
        m_taskbar_data.text_colors.begin()->second.label = m_taskbar_data.dft_text_colors;
    }

    FontInfo default_font;
    default_font.name = m_str_table.GetLanguageInfo().default_font_name.c_str();
    default_font.size = 9;
    ini.LoadFontData(_T("task_bar"), m_taskbar_data.font, default_font);

    //Load display text settings
    ini.LoadDisplayStr(L"task_bar", m_taskbar_data.disp_str, false);
    ini.LoadPluginDisplayStr(L"plugin_display_str_taskbar_window", m_taskbar_data.disp_str, false);

    m_taskbar_data.tbar_wnd_on_left = ini.GetBool(_T("task_bar"), _T("task_bar_wnd_on_left"), false);
    m_taskbar_data.speed_short_mode = ini.GetBool(_T("task_bar"), _T("task_bar_speed_short_mode"), true);
    m_taskbar_data.tbar_wnd_snap = ini.GetBool(_T("task_bar"), _T("task_bar_wnd_snap"), false);
    m_taskbar_data.unit_byte = ini.GetBool(_T("task_bar"), _T("unit_byte"), true);
    m_taskbar_data.speed_unit = static_cast<SpeedUnit>(ini.GetInt(_T("task_bar"), _T("task_bar_speed_unit"), 0));
    m_taskbar_data.hide_unit = ini.GetBool(_T("task_bar"), _T("task_bar_hide_unit"), false);
    m_taskbar_data.hide_percent = ini.GetBool(_T("task_bar"), _T("task_bar_hide_percent"), false);
    m_taskbar_data.value_right_align = ini.GetBool(_T("task_bar"), _T("value_right_align"), false);
    m_taskbar_data.dynamic_color = ini.GetBool(_T("task_bar"), _T("dynamic_color"), true);
    m_taskbar_data.horizontal_arrange = ini.GetBool(_T("task_bar"), _T("horizontal_arrange"), false);
    m_taskbar_data.show_status_bar = ini.GetBool(_T("task_bar"), _T("show_status_bar"), true);
    m_taskbar_data.separate_value_unit_with_space = ini.GetBool(_T("task_bar"), _T("separate_value_unit_with_space"), false);
    m_taskbar_data.show_tool_tip = ini.GetBool(_T("task_bar"), _T("show_tool_tip"), true);
    m_taskbar_data.digits_number = ini.GetInt(_T("task_bar"), _T("digits_number"), 3);
    m_taskbar_data.memory_display = static_cast<MemoryDisplay>(ini.GetInt(L"task_bar", L"memory_display", static_cast<int>(MemoryDisplay::USAGE_PERCENTAGE)));
    m_taskbar_data.double_click_action = static_cast<DoubleClickAction>(ini.GetInt(_T("task_bar"), _T("double_click_action"), 0));
    m_taskbar_data.double_click_exe = ini.GetString(L"task_bar", L"double_click_exe", (theApp.m_system_dir + L"\\Taskmgr.exe").c_str());
    m_taskbar_data.cm_graph_type = ini.GetBool(_T("task_bar"), _T("cm_graph_type"), true);
    m_taskbar_data.show_graph_dashed_box = ini.GetBool(L"task_bar", L"show_graph_dashed_box", false);
    m_taskbar_data.item_space = ini.GetInt(L"task_bar", L"item_space", 8);
    m_taskbar_data.vertical_margin = ini.GetInt(L"task_bar", L"vertical_margin", 0);
    m_taskbar_data.window_offset_top = ini.GetInt(L"task_bar", L"window_offset_top", 0);
    m_taskbar_data.window_offset_left = ini.GetInt(L"task_bar", L"window_offset_left", 0);
    m_taskbar_data.ValidItemSpace();
    m_taskbar_data.ValidWindowOffsetTop();
    m_taskbar_data.ValidWindowOffsetLeft();
    m_taskbar_data.avoid_overlap_with_widgets = ini.GetBool(_T("task_bar"), _T("avoid_overlap_with_widgets"), false);
    m_taskbar_data.taskbar_left_space_win11 = ini.GetInt(L"task_bar", L"taskbar_left_space_win11", 160);
    m_taskbar_data.taskbar_right_space_win11 = ini.GetInt(L"task_bar", L"taskbar_right_space_win11", 280);

    if (m_win_version.IsWindows10OrLater())     //Only Win10 supports automatically adapting to the system dark/light theme
        m_taskbar_data.auto_adapt_light_theme = ini.GetBool(L"task_bar", L"auto_adapt_light_theme", false);
    else
        m_taskbar_data.auto_adapt_light_theme = false;
    m_taskbar_data.dark_default_style = ini.GetInt(L"task_bar", L"dark_default_style", 0);
    m_taskbar_data.light_default_style = ini.GetInt(L"task_bar", L"light_default_style", TASKBAR_DEFAULT_LIGHT_STYLE_INDEX);

    if (m_win_version.IsWindows8OrLater())
        m_taskbar_data.auto_set_background_color = ini.GetBool(L"task_bar", L"auto_set_background_color", false);
    else
        m_taskbar_data.auto_set_background_color = false;

    m_taskbar_data.item_order.Init();
    m_taskbar_data.item_order.FromString(ini.GetString(L"task_bar", L"item_order", L""));
    m_taskbar_data.plugin_display_item.FromString(ini.GetString(L"task_bar", L"plugin_display_item", L""));
    m_taskbar_data.auto_save_taskbar_color_settings_to_preset = ini.GetBool(L"task_bar", L"auto_save_taskbar_color_settings_to_preset", true);

    m_taskbar_data.show_netspeed_figure = ini.GetBool(L"task_bar", L"show_netspeed_figure", true);
    m_taskbar_data.netspeed_figure_max_value = ini.GetInt(L"task_bar", L"netspeed_figure_max_value", 10);
    m_taskbar_data.netspeed_figure_max_value_unit = ini.GetInt(L"task_bar", L"netspeed_figure_max_value_unit", 1);
    m_taskbar_data.graph_color_following_system = ini.GetBool(L"task_bar", L"graph_color_following_system", true);

    if (CTaskBarDlgDrawCommonSupport::CheckSupport())
        m_taskbar_data.disable_d2d = ini.GetBool(L"task_bar", L"disable_d2d", true);
    else
        m_taskbar_data.disable_d2d = true;
    m_taskbar_data.enable_colorful_emoji = ini.GetBool(L"task_bar", L"enable_colorful_emoji", true);

    //Other settings
    //m_cfg_data.m_show_internet_ip = ini.GetBool(L"connection_details", L"show_internet_ip", false);
    m_cfg_data.m_use_log_scale = ini.GetBool(_T("histroy_traffic"), _T("use_log_scale"), true);
    m_cfg_data.m_sunday_first = ini.GetBool(_T("histroy_traffic"), _T("sunday_first"), true);
    m_cfg_data.m_view_type = static_cast<HistoryTrafficViewType>(ini.GetInt(_T("histroy_traffic"), _T("view_type"), static_cast<int>(HistoryTrafficViewType::HV_DAY)));

    m_notify_interval = ini.GetInt(_T("other"), _T("notify_interval"), 60);
    m_exit_when_start_by_restart_manager = ini.GetBool(_T("other"), _T("exit_when_start_by_restart_manager"), true);
    m_debug_log = ini.GetBool(_T("other"), _T("debug_log"), false);
    //Since setting the transparent color of the taskbar window in Win7 system will cause the taskbar window to be invisible, the transparent color setting is disabled in Win7 by default.
    m_taksbar_transparent_color_enable = ini.GetBool(L"other", L"taksbar_transparent_color_enable", !m_win_version.IsWindows7());
    m_last_light_mode = ini.GetBool(L"other", L"last_light_mode", CWindowsSettingHelper::IsWindows10LightTheme());
    m_show_mouse_panetrate_tip = ini.GetBool(L"other", L"show_mouse_panetrate_tip", true);
    m_show_dot_net_notinstalled_tip = ini.GetBool(L"other", L"show_dot_net_notinstalled_tip", true);
}

void CTrafficMonitorApp::SaveConfig()
{
    CSettingsHelper ini;

    //General settings
    ini.WriteBool(_T("general"), _T("check_update_when_start"), m_general_data.check_update_when_start);
    //ini.WriteBool(_T("general"), _T("allow_skin_cover_font"), m_general_data.allow_skin_cover_font);
    //ini.WriteBool(_T("general"), _T("allow_skin_cover_text"), m_general_data.allow_skin_cover_text);
    ini.WriteString(_T("general"), _T("language"), m_general_data.language.toConfigString());
    ini.WriteInt(L"general", L"update_source", m_general_data.update_source);
    ini.WriteBool(L"general", L"show_all_interface", m_general_data.show_all_interface);
    ini.WriteInt(L"general", L"cpu_usage_acquire_method", m_general_data.cpu_usage_acquire_method);
    ini.WriteInt(L"general", L"monitor_time_span", m_general_data.monitor_time_span);
    ini.WriteString(L"general", L"hard_disk_name", m_general_data.hard_disk_name);
    ini.WriteString(L"general", L"cpu_core_name", m_general_data.cpu_core_name);
    ini.WriteInt(L"general", L"hardware_monitor_item", m_general_data.hardware_monitor_item);
    ini.WriteStringList(L"general", L"connections_hide", m_general_data.connections_hide.ToVector());

    //Main window settings
    ini.WriteInt(L"config", L"transparency", m_cfg_data.m_transparency);
    ini.WriteBool(L"config", L"always_on_top", m_main_wnd_data.m_always_on_top);
    ini.WriteBool(L"config", L"lock_window_pos", m_main_wnd_data.m_lock_window_pos);
    ini.WriteBool(L"config", L"show_notify_icon", m_general_data.show_notify_icon);
    ini.WriteBool(L"config", L"show_cpu_memory", m_cfg_data.m_show_more_info);
    ini.WriteBool(L"config", L"mouse_penetrate", m_main_wnd_data.m_mouse_penetrate);
    ini.WriteBool(L"config", L"show_task_bar_wnd", m_cfg_data.m_show_task_bar_wnd);
    ini.WriteInt(L"config", L"position_x", m_cfg_data.m_position_x);
    ini.WriteInt(L"config", L"position_y", m_cfg_data.m_position_y);
    ini.WriteBool(L"connection", L"auto_select", m_cfg_data.m_auto_select);
    ini.WriteBool(L"connection", L"select_all", m_cfg_data.m_select_all);
    //ini.SaveMainWndColors(L"config", L"text_color", m_main_wnd_data.text_colors);
    //ini.WriteBool(_T("config"), _T("specify_each_item_color"), m_main_wnd_data.specify_each_item_color);
    ini.WriteInt(L"config", L"hide_main_window", m_cfg_data.m_hide_main_window);
    ini.WriteString(L"connection", L"connection_name", CCommon::StrToUnicode(m_cfg_data.m_connection_name.c_str()));
    ini.WriteString(_T("config"), _T("skin_selected"), m_cfg_data.m_skin_name.c_str());

    ini.WriteBool(L"skins", L"skin_auto_adapt", m_cfg_data.skin_auto_adapt);
    ini.WriteString(L"skins", L"skin_name_dark_mode", m_cfg_data.skin_name_dark_mode);
    ini.WriteString(L"skins", L"skin_name_light_mode", m_cfg_data.skin_name_light_mode);

    ini.WriteInt(L"config", L"notify_icon_selected", m_cfg_data.m_notify_icon_selected);
    ini.WriteBool(L"config", L"notify_icon_auto_adapt", m_cfg_data.m_notify_icon_auto_adapt);

    //ini.SaveFontData(L"config", m_main_wnd_data.font);

    ini.WriteBool(L"config", L"swap_up_down", m_main_wnd_data.swap_up_down);
    ini.WriteBool(L"config", L"hide_main_wnd_when_fullscreen", m_main_wnd_data.hide_main_wnd_when_fullscreen);

    //ini.SaveDisplayStr(L"config", m_main_wnd_data.disp_str);
    //ini.SavePluginDisplayStr(L"plugin_display_str_main_window", m_main_wnd_data.disp_str);

    ini.WriteBool(L"config", L"speed_short_mode", m_main_wnd_data.speed_short_mode);
    ini.WriteBool(L"config", L"separate_value_unit_with_space", m_main_wnd_data.separate_value_unit_with_space);
    ini.WriteBool(L"config", L"show_tool_tip", m_main_wnd_data.show_tool_tip);
    ini.WriteInt(L"config", L"memory_display", static_cast<int>(m_main_wnd_data.memory_display));
    ini.WriteBool(L"config", L"unit_byte", m_main_wnd_data.unit_byte);
    ini.WriteInt(L"config", L"speed_unit", static_cast<int>(m_main_wnd_data.speed_unit));
    ini.WriteBool(L"config", L"hide_unit", m_main_wnd_data.hide_unit);
    ini.WriteBool(L"config", L"hide_percent", m_main_wnd_data.hide_percent);
    ini.WriteInt(L"config", L"double_click_action", static_cast<int>(m_main_wnd_data.double_click_action));
    ini.WriteString(L"config", L"double_click_exe", m_main_wnd_data.double_click_exe);

    ini.WriteInt(L"config", L"alow_out_of_border", m_main_wnd_data.m_alow_out_of_border);

    ini.WriteBool(L"notify_tip", L"traffic_tip_enable", m_general_data.traffic_tip_enable);
    ini.WriteInt(L"notify_tip", L"traffic_tip_value", m_general_data.traffic_tip_value);
    ini.WriteInt(L"notify_tip", L"traffic_tip_unit", m_general_data.traffic_tip_unit);
    ini.WriteBool(L"notify_tip", L"memory_usage_tip_enable", m_general_data.memory_usage_tip.enable);
    ini.WriteInt(L"notify_tip", L"memory_tip_value", m_general_data.memory_usage_tip.tip_value);
    ini.WriteBool(L"notify_tip", L"cpu_temperature_tip_enable", m_general_data.cpu_temp_tip.enable);
    ini.WriteInt(L"notify_tip", L"cpu_temperature_tip_value", m_general_data.cpu_temp_tip.tip_value);
    ini.WriteBool(L"notify_tip", L"gpu_temperature_tip_enable", m_general_data.gpu_temp_tip.enable);
    ini.WriteInt(L"notify_tip", L"gpu_temperature_tip_value", m_general_data.gpu_temp_tip.tip_value);
    ini.WriteBool(L"notify_tip", L"hdd_temperature_tip_enable", m_general_data.hdd_temp_tip.enable);
    ini.WriteInt(L"notify_tip", L"hdd_temperature_tip_value", m_general_data.hdd_temp_tip.tip_value);
    ini.WriteBool(L"notify_tip", L"mainboard_temperature_tip_enable", m_general_data.mainboard_temp_tip.enable);
    ini.WriteInt(L"notify_tip", L"mainboard_temperature_tip_value", m_general_data.mainboard_temp_tip.tip_value);

    //Taskbar window settings
    ini.WriteInt(L"task_bar", L"task_bar_back_color", m_taskbar_data.back_color);
    ini.WriteInt(L"task_bar", L"transparent_color", m_taskbar_data.transparent_color);
    ini.WriteInt(L"task_bar", L"status_bar_color", m_taskbar_data.status_bar_color);
    ini.SaveTaskbarWndColors(L"task_bar", L"task_bar_text_color", m_taskbar_data.text_colors);
    ini.WriteBool(L"task_bar", L"specify_each_item_color", m_taskbar_data.specify_each_item_color);
    //ini.WriteBool(L"task_bar", L"task_bar_show_cpu_memory", m_cfg_data.m_tbar_show_cpu_memory);
    ini.WriteInt(L"task_bar", L"tbar_display_item", m_taskbar_data.display_item.ToInt());
    ini.SaveFontData(L"task_bar", m_taskbar_data.font);
    //ini.WriteBool(L"task_bar", L"task_bar_swap_up_down", m_taskbar_data.swap_up_down);
    ini.WriteBool(L"task_bar", L"show_taskbar_wnd_in_secondary_display", m_taskbar_data.show_taskbar_wnd_in_secondary_display);
    ini.WriteInt(L"task_bar", L"secondary_display_index", m_taskbar_data.secondary_display_index);

    ini.SaveDisplayStr(L"task_bar", m_taskbar_data.disp_str);
    ini.SavePluginDisplayStr(L"plugin_display_str_taskbar_window", m_taskbar_data.disp_str);

    ini.WriteBool(L"task_bar", L"task_bar_wnd_on_left", m_taskbar_data.tbar_wnd_on_left);
    ini.WriteBool(L"task_bar", L"task_bar_wnd_snap", m_taskbar_data.tbar_wnd_snap);
    ini.WriteBool(L"task_bar", L"task_bar_speed_short_mode", m_taskbar_data.speed_short_mode);
    ini.WriteBool(L"task_bar", L"unit_byte", m_taskbar_data.unit_byte);
    ini.WriteInt(L"task_bar", L"task_bar_speed_unit", static_cast<int>(m_taskbar_data.speed_unit));
    ini.WriteBool(L"task_bar", L"task_bar_hide_unit", m_taskbar_data.hide_unit);
    ini.WriteBool(L"task_bar", L"task_bar_hide_percent", m_taskbar_data.hide_percent);
    ini.WriteBool(L"task_bar", L"value_right_align", m_taskbar_data.value_right_align);
    ini.WriteBool(L"task_bar", L"dynamic_color", m_taskbar_data.dynamic_color);
    ini.WriteBool(L"task_bar", L"horizontal_arrange", m_taskbar_data.horizontal_arrange);
    ini.WriteBool(L"task_bar", L"show_status_bar", m_taskbar_data.show_status_bar);
    ini.WriteBool(L"task_bar", L"separate_value_unit_with_space", m_taskbar_data.separate_value_unit_with_space);
    ini.WriteBool(L"task_bar", L"show_tool_tip", m_taskbar_data.show_tool_tip);
    ini.WriteInt(L"task_bar", L"digits_number", m_taskbar_data.digits_number);
    ini.WriteInt(L"task_bar", L"memory_display", static_cast<int>(m_taskbar_data.memory_display));
    ini.WriteInt(L"task_bar", L"double_click_action", static_cast<int>(m_taskbar_data.double_click_action));
    ini.WriteString(L"task_bar", L"double_click_exe", m_taskbar_data.double_click_exe);
    ini.WriteBool(L"task_bar", L"cm_graph_type", m_taskbar_data.cm_graph_type);
    ini.WriteBool(L"task_bar", L"show_graph_dashed_box", m_taskbar_data.show_graph_dashed_box);
    ini.WriteInt(L"task_bar", L"item_space", m_taskbar_data.item_space);
    ini.WriteInt(L"task_bar", L"vertical_margin", m_taskbar_data.vertical_margin);
    ini.WriteInt(L"task_bar", L"window_offset_top", m_taskbar_data.window_offset_top);
    ini.WriteInt(L"task_bar", L"window_offset_left", m_taskbar_data.window_offset_left);
    ini.WriteBool(L"task_bar", L"avoid_overlap_with_widgets", m_taskbar_data.avoid_overlap_with_widgets);
    ini.WriteInt(L"task_bar", L"taskbar_left_space_win11", m_taskbar_data.taskbar_left_space_win11);
    ini.WriteInt(L"task_bar", L"taskbar_right_space_win11", m_taskbar_data.taskbar_right_space_win11);

    ini.WriteBool(L"task_bar", L"auto_adapt_light_theme", m_taskbar_data.auto_adapt_light_theme);
    ini.WriteInt(L"task_bar", L"dark_default_style", m_taskbar_data.dark_default_style);
    ini.WriteInt(L"task_bar", L"light_default_style", m_taskbar_data.light_default_style);
    ini.WriteBool(L"task_bar", L"auto_set_background_color", m_taskbar_data.auto_set_background_color);

    ini.WriteString(L"task_bar", L"item_order", m_taskbar_data.item_order.ToString());
    ini.WriteString(L"task_bar", L"plugin_display_item", m_taskbar_data.plugin_display_item.ToString());
    ini.WriteBool(L"task_bar", L"auto_save_taskbar_color_settings_to_preset", m_taskbar_data.auto_save_taskbar_color_settings_to_preset);

    ini.WriteBool(L"task_bar", L"show_netspeed_figure", m_taskbar_data.show_netspeed_figure);
    ini.WriteInt(L"task_bar", L"netspeed_figure_max_value", m_taskbar_data.netspeed_figure_max_value);
    ini.WriteInt(L"task_bar", L"netspeed_figure_max_value_unit", m_taskbar_data.netspeed_figure_max_value_unit);
    ini.WriteBool(L"task_bar", L"graph_color_following_system", m_taskbar_data.graph_color_following_system);

    ini.WriteBool(L"task_bar", L"disable_d2d", m_taskbar_data.disable_d2d);
    ini.WriteBool(L"task_bar", L"enable_colorful_emoji", m_taskbar_data.enable_colorful_emoji);

    //Other settings
    //ini.WriteBool(L"connection_details", L"show_internet_ip", m_cfg_data.m_show_internet_ip);
    ini.WriteBool(L"histroy_traffic", L"use_log_scale", m_cfg_data.m_use_log_scale);
    ini.WriteBool(L"histroy_traffic", L"sunday_first", m_cfg_data.m_sunday_first);
    ini.WriteInt(L"histroy_traffic", L"view_type", static_cast<int>(m_cfg_data.m_view_type));

    ini.WriteBool(_T("other"), _T("no_multistart_warning"), m_no_multistart_warning);
    ini.WriteBool(_T("other"), _T("exit_when_start_by_restart_manager"), m_exit_when_start_by_restart_manager);
    ini.WriteBool(_T("other"), _T("debug_log"), m_debug_log);
    ini.WriteInt(_T("other"), _T("notify_interval"), m_notify_interval);
    ini.WriteBool(_T("other"), _T("taksbar_transparent_color_enable"), m_taksbar_transparent_color_enable);
    ini.WriteBool(_T("other"), _T("last_light_mode"), m_last_light_mode);
    ini.WriteBool(_T("other"), _T("show_mouse_panetrate_tip"), m_show_mouse_panetrate_tip);
    ini.WriteBool(_T("other"), _T("show_dot_net_notinstalled_tip"), m_show_dot_net_notinstalled_tip);

    ini.WriteString(L"config", L"plugin_disabled", m_cfg_data.plugin_disabled.ToString());

    ini.WriteString(L"app", L"version", VERSION);

    //Check if saving is successful
    if (!ini.Save())
    {
        if (m_cannot_save_config_warning)
        {
            CString info = CCommon::LoadText(IDS_CONNOT_SAVE_CONFIG_WARNING);
            info.Replace(_T("<%file_path%>"), m_config_path.c_str());
            AfxMessageBox(info, MB_ICONWARNING);
        }
        m_cannot_save_config_warning = false;
        return;
    }
}

void CTrafficMonitorApp::LoadPluginDisabledSettings()
{
    CIniHelper ini{ m_config_path };
    m_cfg_data.plugin_disabled.FromString(ini.GetString(L"config", L"plugin_disabled", L""));
}

void CTrafficMonitorApp::LoadGlobalConfig()
{
    bool portable_mode_default{ false };
    wstring global_cfg_path{ m_module_dir + L"global_cfg.ini" };
    if (!CCommon::FileExist(global_cfg_path.c_str()))       //If global_cfg.ini does not exist, determine the storage location of the configuration file based on whether config.ini exists in the AppData/Roaming/TrafficMonitor directory.
    {
        portable_mode_default = !CCommon::FileExist((m_appdata_dir + L"config.ini").c_str());
    }

    CIniHelper ini{ global_cfg_path };
    m_general_data.portable_mode = ini.GetBool(L"config", L"portable_mode", portable_mode_default);

    //Perform a save operation to check whether the current directory has write permissions
    m_module_dir_writable = ini.Save();

    if (m_module_dir.find(CCommon::GetTemplateDir()) != wstring::npos)      //If the current path is in the Temp directory, force the data to be saved to Appdata
    {
        m_module_dir_writable = false;
    }

    if (!m_module_dir_writable)              //If the current directory does not have write permission, the setting configuration is saved to the AppData directory.
    {
        m_general_data.portable_mode = false;
    }
}

void CTrafficMonitorApp::SaveGlobalConfig()
{
    CIniHelper ini{ m_module_dir + L"global_cfg.ini" };
    ini.WriteBool(L"config", L"portable_mode", m_general_data.portable_mode);

    //Check if saving is successful
    if (!ini.Save())
    {
        //if (m_cannot_save_global_config_warning)
        //{
        //    CString info;
        //    info.LoadString(IDS_CONNOT_SAVE_CONFIG_WARNING);
        //    info.Replace(_T("<%file_path%>"), m_module_dir.c_str());
        //    AfxMessageBox(info, MB_ICONWARNING);
        //}
        //m_cannot_save_global_config_warning = false;
        //return;
    }
}

int CTrafficMonitorApp::DPI(int pixel)
{
    return m_dpi * pixel / 96;
}

void CTrafficMonitorApp::DPI(CRect& rect)
{
    rect.left = DPI(rect.left);
    rect.right = DPI(rect.right);
    rect.top = DPI(rect.top);
    rect.bottom = DPI(rect.bottom);
}

void CTrafficMonitorApp::DPIFromWindow(CWnd* pWnd)
{
    CWindowDC dc(pWnd);
    HDC hDC = dc.GetSafeHdc();
    m_dpi = GetDeviceCaps(hDC, LOGPIXELSY);
}

void CTrafficMonitorApp::CheckUpdate(bool message)
{
    if (m_checking_update)      //If you are still checking for updates, return directly
        return;
    CFlagLocker update_locker(m_checking_update);
    CWaitCursor wait_cursor;

    wstring version;        //Program version
    wstring link;           //Download link
    wstring contents_zh_cn; //Update content (Simplified Chinese)
    wstring contents_en;    //Update content（English）
    wstring contents_zh_tw; //Update content (Traditional Chinese)
    CUpdateHelper update_helper;
    update_helper.SetUpdateSource(static_cast<CUpdateHelper::UpdateSource>(m_general_data.update_source));
    if (!update_helper.CheckForUpdate())
    {
        if (message)
            AfxMessageBox(CCommon::LoadText(IDS_CHECK_UPDATE_FAILD), MB_OK | MB_ICONWARNING);
        return;
    }
    version = update_helper.GetVersion();
#ifdef _M_X64
    link = update_helper.GetLink64();
#elif defined _M_ARM64EC
    link = update_helper.GetLinkArm64ec();
#else
    link = update_helper.GetLink();
#endif
    contents_zh_cn = update_helper.GetContentsZhCn();
    contents_en = update_helper.GetContentsEn();
    contents_zh_tw = update_helper.GetContentsZhTw();
    if (version.empty() || link.empty())
    {
        if (message)
        {
            CString info = CCommon::LoadText(IDS_CHECK_UPDATE_ERROR);
            info += _T("\r\nrow_data=");
            info += std::to_wstring(update_helper.IsRowData()).c_str();

            AfxMessageBox(info, MB_OK | MB_ICONWARNING);
        }
        return;
    }
    if (version > VERSION)      //If the version on the server is greater than the local version
    {
        CString info;
        //Select the updated content of the corresponding language version according to the language settings
        wstring language_tag = m_str_table.GetLanguageInfo().bcp_47;
        wstring contents_lan;
        if (language_tag == L"zh-CN")
            contents_lan = contents_zh_cn;
        else if (language_tag == L"zh-TW")
            contents_lan = contents_zh_tw;
        else
            contents_lan = contents_en;
        if (contents_lan.empty())
            info.Format(CCommon::LoadText(IDS_UPDATE_AVLIABLE), version.c_str());
        else
            info.Format(CCommon::LoadText(IDS_UPDATE_AVLIABLE2), version.c_str(), contents_lan.c_str());

        if (AfxMessageBox(info, MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            ShellExecute(NULL, _T("open"), link.c_str(), NULL, NULL, SW_SHOW);      //Go to download link
        }
    }
    else
    {
        if (message)
            AfxMessageBox(CCommon::LoadText(IDS_ALREADY_UPDATED), MB_OK | MB_ICONINFORMATION);
    }
}

void CTrafficMonitorApp::CheckUpdateInThread(bool message)
{
    AfxBeginThread(CheckUpdateThreadFunc, (LPVOID)message);
}

UINT CTrafficMonitorApp::CheckUpdateThreadFunc(LPVOID lpParam)
{
    CCommon::SetThreadLanguage(theApp.m_general_data.language.language_id);     //Set thread language
#ifndef _DEBUG      //DEBUG does not check for updates at startup
    theApp.CheckUpdate(lpParam);        //Check for updates
#endif
    //Detect plugin updates
    if (!theApp.m_plugins.GetPlugins().empty())
    {
        theApp.m_plugin_update.CheckForUpdate();
    }
    return 0;
}

UINT CTrafficMonitorApp::InitOpenHardwareMonitorLibThreadFunc(LPVOID lpParam)
{
#ifndef WITHOUT_TEMPERATURE
    CSingleLock sync(&theApp.m_minitor_lib_critical, TRUE);
    theApp.m_pMonitor = OpenHardwareMonitorApi::CreateInstance();
    if (theApp.m_pMonitor == nullptr)
    {
        AfxMessageBox(OpenHardwareMonitorApi::GetErrorMessage().c_str(), MB_ICONERROR | MB_OK);
    }
    //Set the enabled status of hardware monitoring
    theApp.UpdateOpenHardwareMonitorEnableState();
#endif
    return 0;
}

bool  CTrafficMonitorApp::SetAutoRun(bool auto_run, bool task_scheduler)
{
    if (task_scheduler)
    {
        //Use task scheduling to set up auto-start at boot
        return SetAutoRunByTaskScheduler(auto_run);
    }
    else
    {
        //Use the method of adding registry entries to achieve automatic startup at boot
        return SetAutoRunByRegistry(auto_run);
    }
}

bool CTrafficMonitorApp::GetAutoRun(wstring* auto_run_path, bool task_scheduler)
{
    if (auto_run_path != nullptr)
        auto_run_path->clear();

    if (task_scheduler)
    {
        //Use task scheduling to achieve automatic startup at boot
        return is_auto_start_task_active_for_this_user(auto_run_path);
    }
    else
    {
        //Use the method of adding registry entries to achieve automatic startup at boot
        CRegKey key;
        if (key.Open(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run")) != ERROR_SUCCESS)
        {
            //If opening the registry "Software\\Microsoft\\Windows\\CurrentVersion\\Run" fails, false will be returned.
            return false;
        }
        wchar_t buff[256];
        ULONG size{ 256 };
        if (key.QueryStringValue(APP_NAME, buff, &size) == ERROR_SUCCESS)       //If the "TrafficMonitor" key is found
        {
            if (auto_run_path != nullptr)
            {
                //save path
                *auto_run_path = buff;
                //Remove the quotes around
                if (auto_run_path->front() == L'\"')
                    *auto_run_path = auto_run_path->substr(1);
                if (auto_run_path->back() = L'\"')
                    auto_run_path->pop_back();
            }
            return (m_module_path_reg == buff); //If the value of "TrafficMonitor" is the path of the current program, it returns true, otherwise it returns false.
        }
        else
        {
            return false;       //"TrafficMonitor" key not found, returns false
        }
    }
}

bool CTrafficMonitorApp::SetAutoRunByRegistry(bool auto_run)
{
    CRegKey key;
    //Open registry key
    if (key.Open(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run")) != ERROR_SUCCESS)
    {
        AfxMessageBox(CCommon::LoadText(IDS_AUTORUN_FAILED_NO_KEY), MB_OK | MB_ICONWARNING);
        return false;
    }
    if (auto_run)       //Write registry key
    {
        //Delete the auto-start item in the scheduled task when setting the auto-start item at startup through the registry
        SetAutoRunByTaskScheduler(false);

        if (key.SetStringValue(APP_NAME, m_module_path_reg.c_str()) != ERROR_SUCCESS)
        {
            AfxMessageBox(CCommon::LoadText(IDS_AUTORUN_FAILED_NO_ACCESS), MB_OK | MB_ICONWARNING);
            return false;
        }
    }
    else        //Delete registry key
    {
        //Before deleting, check whether the registry key exists. If it does not exist, return directly.
        wchar_t buff[256];
        ULONG size{ 256 };
        if (key.QueryStringValue(APP_NAME, buff, &size) != ERROR_SUCCESS)
            return false;
        if (key.DeleteValue(APP_NAME) != ERROR_SUCCESS)
        {
            AfxMessageBox(CCommon::LoadText(IDS_AUTORUN_DELETE_FAILED), MB_OK | MB_ICONWARNING);
            return false;
        }
    }
    return true;
}

bool CTrafficMonitorApp::SetAutoRunByTaskScheduler(bool auto_run)
{
    bool succeed = delete_auto_start_task_for_this_user();     //Delete auto-start at boot first
    if (auto_run)
    {
        //Delete the auto-start items in the registry when setting the auto-start items at startup through scheduled tasks
        SetAutoRunByRegistry(false);

        succeed = create_auto_start_task_for_this_user(true);
    }
    return succeed;
}

CString CTrafficMonitorApp::GetSystemInfoString()
{
    CString info;
    info += _T("System Info:\r\n");

    CString strTmp;
    strTmp.Format(_T("Windows Version: %d.%d build %d"), m_win_version.GetMajorVersion(),
        m_win_version.GetMinorVersion(), m_win_version.GetBuildNumber());
    info += strTmp;

    if (m_win_version.IsWine())
        info += _T(" (Wine)");

    info += _T("\r\n");

    strTmp.Format(_T("DPI: %d"), m_dpi);
    info += strTmp;
    info += _T("\r\n");

    info += _T("Version: ");
    info += VERSION;
    info += _T(" ");
#ifdef _M_X64
    info += _T("x64");
#elif defined _M_ARM64EC
    info += _T("Arm64EC");
#else
    info += _T("x86");
#endif

#ifdef WITHOUT_TEMPERATURE
    info += _T(" (Lite)");
#endif

    info += _T("\r\nLast compiled date: ");
    info += CCommon::GetLastCompileTime();

    return info;
}


void CTrafficMonitorApp::InitMenuResourse()
{
    //loading menu
    CCommon::LoadMenuResource(m_main_menu, IDR_MENU1);
    CCommon::LoadMenuResource(m_main_menu_plugin, IDR_MENU1);
    CCommon::LoadMenuResource(m_taskbar_menu, IDR_TASK_BAR_MENU);
    CCommon::LoadMenuResource(m_taskbar_menu_plugin, IDR_TASK_BAR_MENU);

    //Add extra items to plugin menu
    m_main_menu_plugin_sub_menu.CreatePopupMenu();
    m_main_menu_plugin_sub_menu.AppendMenu(MF_STRING | MF_ENABLED, ID_PLUGIN_OPTIONS, CCommon::LoadText(IDS_PLUGIN_OPTIONS, _T("...")));
    m_main_menu_plugin_sub_menu.AppendMenu(MF_STRING | MF_ENABLED, ID_PLUGIN_DETAIL, CCommon::LoadText(IDS_PLUGIN_INFO, _T("...")));
    CMenuIcon::AddIconToMenuItem(m_main_menu_plugin_sub_menu.GetSafeHmenu(), ID_PLUGIN_OPTIONS, FALSE, GetMenuIcon(IDI_SETTINGS));
    CMenuIcon::AddIconToMenuItem(m_main_menu_plugin_sub_menu.GetSafeHmenu(), ID_PLUGIN_DETAIL, FALSE, GetMenuIcon(IDI_INFO));
    CMenu* main_menu_plugin = m_main_menu_plugin.GetSubMenu(0);
    main_menu_plugin->AppendMenu(MF_SEPARATOR);
    main_menu_plugin->AppendMenu(MF_POPUP | MF_STRING, (UINT)m_main_menu_plugin_sub_menu.m_hMenu, _T("<plugin name>"));

    m_taskbar_menu_plugin_sub_menu.CreatePopupMenu();
    m_taskbar_menu_plugin_sub_menu.AppendMenu(MF_STRING | MF_ENABLED, ID_PLUGIN_OPTIONS_TASKBAR, CCommon::LoadText(IDS_PLUGIN_OPTIONS, _T("...")));
    m_taskbar_menu_plugin_sub_menu.AppendMenu(MF_STRING | MF_ENABLED, ID_PLUGIN_DETAIL_TASKBAR, CCommon::LoadText(IDS_PLUGIN_INFO, _T("...")));
    CMenuIcon::AddIconToMenuItem(m_taskbar_menu_plugin_sub_menu.GetSafeHmenu(), ID_PLUGIN_OPTIONS_TASKBAR, FALSE, GetMenuIcon(IDI_SETTINGS));
    CMenuIcon::AddIconToMenuItem(m_taskbar_menu_plugin_sub_menu.GetSafeHmenu(), ID_PLUGIN_DETAIL_TASKBAR, FALSE, GetMenuIcon(IDI_INFO));
    CMenu* taskbar_menu_plugin = m_taskbar_menu_plugin.GetSubMenu(0);
    taskbar_menu_plugin->AppendMenu(MF_SEPARATOR);
    taskbar_menu_plugin->AppendMenu(MF_POPUP | MF_STRING, (UINT)m_taskbar_menu_plugin_sub_menu.m_hMenu, _T("<plugin name>"));

    //Add icons to menu items
    auto addIconsForMainWindowMenu = [&](const CMenu& menu)
    {
        CMenuIcon::AddIconToMenuItem(menu.GetSubMenu(0)->GetSafeHmenu(), 0, TRUE, GetMenuIcon(IDI_CONNECTION));
        CMenuIcon::AddIconToMenuItem(menu.GetSubMenu(0)->GetSafeHmenu(), 11, TRUE, GetMenuIcon(IDI_FUNCTION));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_NETWORK_INFO, FALSE, GetMenuIcon(IDI_INFO));
        if (!m_win_version.IsWindows11OrLater())
            CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_ALWAYS_ON_TOP, FALSE, GetMenuIcon(IDI_PIN));
        if (!m_win_version.IsWindows11OrLater())
            CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_MOUSE_PENETRATE, FALSE, GetMenuIcon(IDI_MOUSE));
        if (!m_win_version.IsWindows11OrLater())
            CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_LOCK_WINDOW_POS, FALSE, GetMenuIcon(IDI_LOCK));
        //if (!m_win_version.IsWindows11OrLater())
        //    CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_SHOW_NOTIFY_ICON, FALSE, GetMenuIcon(IDI_NOTIFY));
        if (!m_win_version.IsWindows11OrLater())
            CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_SHOW_CPU_MEMORY, FALSE, GetMenuIcon(IDI_MORE));
        if (!m_win_version.IsWindows11OrLater())
            CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_SHOW_TASK_BAR_WND, FALSE, GetMenuIcon(IDI_TASKBAR_WINDOW));

        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_CHANGE_SKIN, FALSE, GetMenuIcon(IDI_SKIN));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_CHANGE_NOTIFY_ICON, FALSE, GetMenuIcon(IDI_NOTIFY));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_TRAFFIC_HISTORY, FALSE, GetMenuIcon(IDI_STATISTICS));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_PLUGIN_MANAGE, FALSE, GetMenuIcon(IDI_PLUGINS));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_OPTIONS, FALSE, GetMenuIcon(IDI_SETTINGS));
        CMenuIcon::AddIconToMenuItem(menu.GetSubMenu(0)->GetSafeHmenu(), 14, TRUE, GetMenuIcon(IDI_HELP));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_HELP, FALSE, GetMenuIcon(IDI_HELP));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_APP_ABOUT, FALSE, GetMenuIcon(IDR_MAINFRAME));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_APP_EXIT, FALSE, GetMenuIcon(IDI_EXIT));
    };
    //Main window right-click menu
    addIconsForMainWindowMenu(m_main_menu);
    addIconsForMainWindowMenu(m_main_menu_plugin);

    //Taskbar window right-click menu
    auto addIconsForTaksbarWindowMenu = [&](const CMenu& menu)
    {
        CMenuIcon::AddIconToMenuItem(menu.GetSubMenu(0)->GetSafeHmenu(), 0, TRUE, GetMenuIcon(IDI_CONNECTION));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_NETWORK_INFO, FALSE, GetMenuIcon(IDI_INFO));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_TRAFFIC_HISTORY, FALSE, GetMenuIcon(IDI_STATISTICS));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_DISPLAY_SETTINGS, FALSE, GetMenuIcon(IDI_ITEM));
        //if (!m_win_version.IsWindows11OrLater())
        //    CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_SHOW_NOTIFY_ICON, FALSE, GetMenuIcon(IDI_NOTIFY));

        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_SHOW_TASK_BAR_WND, FALSE, GetMenuIcon(IDI_CLOSE));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_OPEN_TASK_MANAGER, FALSE, GetMenuIcon(IDI_TASK_MANAGER));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_OPTIONS2, FALSE, GetMenuIcon(IDI_SETTINGS));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_PLUGIN_MANAGE, FALSE, GetMenuIcon(IDI_PLUGINS));
        CMenuIcon::AddIconToMenuItem(menu.GetSubMenu(0)->GetSafeHmenu(), 13, TRUE, GetMenuIcon(IDI_HELP));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_HELP, FALSE, GetMenuIcon(IDI_HELP));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_APP_ABOUT, FALSE, GetMenuIcon(IDR_MAINFRAME));
        CMenuIcon::AddIconToMenuItem(menu.GetSafeHmenu(), ID_APP_EXIT, FALSE, GetMenuIcon(IDI_EXIT));
    };
    addIconsForTaksbarWindowMenu(m_taskbar_menu);
    addIconsForTaksbarWindowMenu(m_taskbar_menu_plugin);

#ifdef _DEBUG
    m_main_menu.GetSubMenu(0)->AppendMenu(MF_BYCOMMAND, ID_CMD_TEST, _T("Test Command"));
#endif
}

HICON CTrafficMonitorApp::GetMenuIcon(UINT id)
{
    auto iter = m_menu_icons.find(id);
    if (iter != m_menu_icons.end())
    {
        return iter->second;
    }
    else
    {
        HICON hIcon = CCommon::LoadIconResource(id, DPI(16));
        m_menu_icons[id] = hIcon;
        return hIcon;
    }
}

void CTrafficMonitorApp::AutoSelectNotifyIcon()
{
    if (m_win_version.GetMajorVersion() >= 10)
    {
        bool light_mode = CWindowsSettingHelper::IsWindows10LightTheme();
        if (light_mode)     //In light mode, if the icon is white, change it to black
        {
            if (m_cfg_data.m_notify_icon_selected == 0)
                m_cfg_data.m_notify_icon_selected = 4;
            if (m_cfg_data.m_notify_icon_selected == 1)
                m_cfg_data.m_notify_icon_selected = 5;
        }
        else     //In dark mode, if the icon is black, change it to white
        {
            if (m_cfg_data.m_notify_icon_selected == 4)
                m_cfg_data.m_notify_icon_selected = 0;
            if (m_cfg_data.m_notify_icon_selected == 5)
                m_cfg_data.m_notify_icon_selected = 1;
        }
    }
}

// The only CTrafficMonitorApp object

CTrafficMonitorApp theApp;


// CTrafficMonitorApp initialization

BOOL CTrafficMonitorApp::InitInstance()
{
    //Replace the default class name of the dialog box program
    WNDCLASS wc;
    ::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);       //The default class names of all dialog boxes in MFC are #32770
    wc.lpszClassName = APP_CLASS_NAME;      //Change the class name of the dialog box to the new class name
    AfxRegisterClass(&wc);

    //Set the path to the configuration file
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    m_module_path = path;
    if (m_module_path.find(L' ') != wstring::npos)
    {
        //If there are spaces in the path, add double quotes before and after the program path
        m_module_path_reg = L'\"';
        m_module_path_reg += m_module_path;
        m_module_path_reg += L'\"';
    }
    else
    {
        m_module_path_reg = m_module_path;
    }
    m_module_dir = CCommon::GetModuleDir();
    m_system_dir = CCommon::GetSystemDir();
    m_appdata_dir = CCommon::GetAppDataConfigDir();

    LoadGlobalConfig();

#ifdef _DEBUG
    m_config_dir = L".\\";
    m_skin_path = L".\\skins\\";
#else
    if (m_general_data.portable_mode)
        m_config_dir = m_module_dir;
    else
        m_config_dir = m_appdata_dir;
    m_skin_path = m_module_dir + L"skins\\";
#endif
    //Program configuration file path in AppData
    m_config_path = m_config_dir + L"config.ini";
    m_history_traffic_path = m_config_dir + L"history_traffic.dat";
    m_log_path = m_config_dir + L"error.log";

    //#ifndef _DEBUG
    //  //The path to the original configuration file in the directory where the program is located
    //  wstring config_path_old = m_module_dir + L"config.ini";
    //  wstring history_traffic_path_old = m_module_dir + L"history_traffic.dat";
    //  wstring log_path_old = m_module_dir + L"error.log";
    //  //If the directory where the program is located contains a configuration file, move it to the directory corresponding to AppData.
    //  CCommon::MoveAFile(config_path_old.c_str(), m_config_path.c_str());
    //  CCommon::MoveAFile(history_traffic_path_old.c_str(), m_history_traffic_path.c_str());
    //  CCommon::MoveAFile(log_path_old.c_str(), m_log_path.c_str());
    //#endif // !_DEBUG

    LoadLanguageConfig();

    //Initialize interface language
    CCommon::SetThreadLanguage(m_general_data.language.language_id);

    //Initialize string resources
    m_str_table.Init();

    //Check if there is already an instance running
    CIniHelper ini{ m_config_path };
    m_no_multistart_warning = ini.GetBool(_T("other"), _T("no_multistart_warning"), false);
    LPCTSTR mutex_name{};
#ifdef _DEBUG
    mutex_name = _T("TrafficMonitor-e8Ahk24HP6JC8hDy");
#else
    mutex_name = _T("TrafficMonitor-1419J3XLKL1w8OZc");
#endif
    HANDLE hMutex = ::CreateMutex(NULL, TRUE, mutex_name);
    if (hMutex != NULL)
    {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            //char buff[128];
            //string cmd_line_str{ CCommon::UnicodeToStr(cmd_line.c_str()) };
            //sprintf_s(buff, "when_start=%d, m_no_multistart_warning=%d, cmd_line=%s", when_start, m_no_multistart_warning, cmd_line_str.c_str());
            //CCommon::WriteLog(buff, _T(".\\start.log"));
            if (!m_no_multistart_warning)
            {
                //Find the handle of the main window of the existing TrafficMonitor process
                HWND exist_handel = ::FindWindow(APP_CLASS_NAME, NULL);
                if (exist_handel != NULL)
                {
                    //The "TrafficMonitor is already running" dialog box pops up
                    CAppAlreadyRuningDlg dlg(exist_handel);
                    dlg.DoModal();
                }
                else
                {
                    AfxMessageBox(CCommon::LoadText(IDS_AN_INSTANCE_RUNNING));
                }
            }
            return FALSE;
        }
    }

    //Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

    //Use screen DPI initially
    DPIFromWindow(nullptr);

    //Load plugin
    LoadPluginDisabledSettings();
    m_plugins.LoadPlugins();

    //Load settings from ini file
    LoadConfig();

    m_taskbar_default_style.LoadConfig();

    //SaveConfig();

    // If an application running on Windows XP has a manifest that specifies
    // Use ComCtl32.dll version 6 or higher to enable visual mode,
    //InitCommonControlsEx() is required.  Otherwise, the window will not be created.
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    // Set it to include all the
    // Public control class.
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();


    AfxEnableControlContainer();

    // Create a shell manager in case the dialog box contains
    // Any shell tree view control or shell list view control.
    CShellManager* pShellManager = new CShellManager;

    // Activate the "Windows Native" visual manager to enable themes in MFC controls
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

    // Standard initialization
    // If you are not using these features and wish to reduce
    // final executable size, you should remove the following
    // No specific initialization routine required
    // Change the registry key used to store settings
    // TODO: The string should be modified appropriately.
    // For example, change it to a company or organization name
    //SetRegistryKey(_T("TrafficMonitor Application")); //Do not use the registry to save data yet

    //Check for updates on startup
    if (m_general_data.check_update_when_start)
    {
        CheckUpdateInThread(false);
    }

#ifndef WITHOUT_TEMPERATURE
    //Check whether .net framework 4.5 is installed
    if (!CWindowsSettingHelper::IsDotNetFramework4Point5Installed())
    {
        if (theApp.m_show_dot_net_notinstalled_tip)
        {
            if (AfxMessageBox(CCommon::LoadText(IDS_DOTNET_NOT_INSTALLED_TIP), MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL)       //Click "Cancel" to stop prompting
            {
                theApp.m_show_dot_net_notinstalled_tip = false;
                SaveConfig();
            }
        }
    }
    else
    {
        //If no hardware monitoring is enabled, OpenHardwareMonitor will not be initialized.
        if (theApp.m_general_data.IsHardwareEnable(HI_CPU) || theApp.m_general_data.IsHardwareEnable(HI_GPU)
            || theApp.m_general_data.IsHardwareEnable(HI_HDD) || theApp.m_general_data.IsHardwareEnable(HI_MBD))
        {
            //Start the thread that initializes OpenHardwareMonitor. Since the initialization of OpenHardwareMonitor takes a certain amount of time, in order to prevent the program from getting stuck during startup, it is processed in a background thread.
            InitOpenHardwareLibInThread();
        }
    }
#endif

    //Execute test code
#ifdef _DEBUG
    CTest::Test();
#endif

    SendSettingsToPlugin();

    CTrafficMonitorDlg dlg;
    m_pMainWnd = &dlg;
    INT_PTR nResponse = dlg.DoModal();
    if (nResponse == IDOK)
    {
        // TODO: Put processing here when to use it
        //  Code to close the dialog box with "OK"
    }
    else if (nResponse == IDCANCEL)
    {
        // TODO: Put processing here when to use it
        //  "Cancel" code to close the dialog box
    }
    else if (nResponse == -1)
    {
        TRACE(traceAppMsg, 0, "Warning: Dialog creation failed; the application will terminate.\n");
        TRACE(traceAppMsg, 0, "Warning: If using MFC controls in dialogs, cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
    }

    // Delete the shell manager created above.
    if (pShellManager != NULL)
    {
        delete pShellManager;
    }

#ifndef _AFXDLL
    ControlBarCleanUp();
#endif

    // Since the dialog box has been closed, FALSE will be returned to exit the application.
    //  Instead of starting the application's message pump.
    return FALSE;
}

void CTrafficMonitorApp::InitOpenHardwareLibInThread()
{
#ifndef WITHOUT_TEMPERATURE
    AfxBeginThread(InitOpenHardwareMonitorLibThreadFunc, NULL);
#endif
}


void CTrafficMonitorApp::UpdateOpenHardwareMonitorEnableState()
{
#ifndef WITHOUT_TEMPERATURE
    if (m_pMonitor != nullptr)
    {
        CSingleLock sync(&theApp.m_minitor_lib_critical, TRUE);
        m_pMonitor->SetCpuEnable(m_general_data.IsHardwareEnable(HI_CPU));
        m_pMonitor->SetGpuEnable(m_general_data.IsHardwareEnable(HI_GPU));
        m_pMonitor->SetHddEnable(m_general_data.IsHardwareEnable(HI_HDD));
        m_pMonitor->SetMainboardEnable(m_general_data.IsHardwareEnable(HI_MBD));
    }
#endif
}

//void CTrafficMonitorApp::UpdateTaskbarWndMenu()
//{
//    //Get the "Display Settings" submenu
//    CMenu* pMenu = m_taskbar_menu.GetSubMenu(0)->GetSubMenu(5);
//    ASSERT(pMenu != nullptr);
//    if (pMenu != nullptr)
//    {
//        //Delete all menu items after ID_SHOW_MEMORY_USAGE
//        if (pMenu->GetMenuItemCount() > 4)
//        {
//            int start_pos = CCommon::GetMenuItemPosition(pMenu, ID_SHOW_MEMORY_USAGE) + 1;
//            while (pMenu->GetMenuItemCount() > start_pos)
//            {
//                pMenu->DeleteMenu(start_pos, MF_BYPOSITION);
//            }
//        }
//
//        //Add temperature related menu items
//#ifndef WITHOUT_TEMPERATURE
//        if (m_general_data.IsHardwareEnable(HI_GPU))
//            pMenu->AppendMenu(MF_STRING | MF_ENABLED, ID_SHOW_GPU, CCommon::LoadText(IDS_GPU_USAGE));
//        if (m_general_data.IsHardwareEnable(HI_CPU))
//            pMenu->AppendMenu(MF_STRING | MF_ENABLED, ID_SHOW_CPU_TEMPERATURE, CCommon::LoadText(IDS_CPU_TEMPERATURE));
//        if (m_general_data.IsHardwareEnable(HI_GPU))
//            pMenu->AppendMenu(MF_STRING | MF_ENABLED, ID_SHOW_GPU_TEMPERATURE, CCommon::LoadText(IDS_GPU_TEMPERATURE));
//        if (m_general_data.IsHardwareEnable(HI_HDD))
//            pMenu->AppendMenu(MF_STRING | MF_ENABLED, ID_SHOW_HDD_TEMPERATURE, CCommon::LoadText(IDS_HDD_TEMPERATURE));
//        if (m_general_data.IsHardwareEnable(HI_MBD))
//            pMenu->AppendMenu(MF_STRING | MF_ENABLED, ID_SHOW_MAIN_BOARD_TEMPERATURE, CCommon::LoadText(IDS_MAINBOARD_TEMPERATURE));
//        if (m_general_data.IsHardwareEnable(HI_HDD))
//            pMenu->AppendMenu(MF_STRING | MF_ENABLED, ID_SHOW_HDD, CCommon::LoadText(IDS_HDD_USAGE));
//#endif
//
//        pMenu->AppendMenu(MF_STRING | MF_ENABLED, ID_SHOW_TOTAL_SPEED, CCommon::LoadText(IDS_TOTAL_NET_SPEED));
//
//        //Add plugin menu item
//        if (!m_plugins.GetPluginItems().empty())
//            pMenu->AppendMenu(MF_SEPARATOR);
//        int plugin_index = 0;
//        for (const auto& plugin_item : m_plugins.GetPluginItems())
//        {
//            pMenu->AppendMenu(MF_STRING | MF_ENABLED, ID_SHOW_PLUGIN_ITEM_START + plugin_index, plugin_item->GetItemName());
//            plugin_index++;
//        }
//    }
//}

bool CTrafficMonitorApp::IsForceShowNotifyIcon()
{
    return ((!m_cfg_data.m_show_task_bar_wnd /*|| m_win_version.IsWindows11OrLater()*/)
        && (m_cfg_data.m_hide_main_window || m_main_wnd_data.m_mouse_penetrate));    //If the taskbar window is not displayed and the main window is hidden or mouse penetration is set, disable the "Show notification area icon" menu item
}

std::wstring CTrafficMonitorApp::GetPlauginTooltipInfo() const
{
    std::wstring tip_info;
    for (const auto& item : m_plugins.GetPlugins())
    {
        if (item.plugin != nullptr && item.plugin->GetAPIVersion() >= 2)
        {
            std::wstring plugin_tooltip = item.plugin->GetTooltipInfo();
            CCommon::StringNormalize(plugin_tooltip);
            if (!plugin_tooltip.empty())
            {
                tip_info += L"\r\n";
                tip_info += plugin_tooltip.c_str();
            }
        }
    }
    return tip_info;
}

bool CTrafficMonitorApp::IsTaksbarItemDisplayed(CommonDisplayItem item) const
{
    if (item.IsPlugin())
    {
        if (item.PluginItem() != nullptr)
            return m_taskbar_data.plugin_display_item.Contains(item.PluginItem()->GetItemId());
    }
    else
    {
        return m_taskbar_data.display_item.Contains(item.ItemType());
    }
    return false;
}

void CTrafficMonitorApp::SendSettingsToPlugin()
{
    for (const auto& plugin_info : m_plugins.GetPlugins())
    {
        if (plugin_info.plugin != nullptr && plugin_info.plugin->GetAPIVersion() >= 2)
        {
            plugin_info.plugin->OnExtenedInfo(ITMPlugin::EI_NAIN_WND_NET_SPEED_SHORT_MODE, std::to_wstring(m_main_wnd_data.speed_short_mode).c_str());
            plugin_info.plugin->OnExtenedInfo(ITMPlugin::EI_MAIN_WND_SPERATE_WITH_SPACE, std::to_wstring(m_main_wnd_data.separate_value_unit_with_space).c_str());
            plugin_info.plugin->OnExtenedInfo(ITMPlugin::EI_MAIN_WND_UNIT_BYTE, std::to_wstring(m_main_wnd_data.unit_byte).c_str());
            plugin_info.plugin->OnExtenedInfo(ITMPlugin::EI_MAIN_WND_UNIT_SELECT, std::to_wstring(static_cast<int>(m_main_wnd_data.speed_unit)).c_str());
            plugin_info.plugin->OnExtenedInfo(ITMPlugin::EI_MAIN_WND_NOT_SHOW_UNIT, std::to_wstring(m_main_wnd_data.hide_unit).c_str());
            plugin_info.plugin->OnExtenedInfo(ITMPlugin::EI_MAIN_WND_NOT_SHOW_PERCENT, std::to_wstring(m_main_wnd_data.hide_percent).c_str());

            plugin_info.plugin->OnExtenedInfo(ITMPlugin::EI_TASKBAR_WND_NET_SPEED_SHORT_MODE, std::to_wstring(m_taskbar_data.speed_short_mode).c_str());
            plugin_info.plugin->OnExtenedInfo(ITMPlugin::EI_TASKBAR_WND_SPERATE_WITH_SPACE, std::to_wstring(m_taskbar_data.separate_value_unit_with_space).c_str());
            plugin_info.plugin->OnExtenedInfo(ITMPlugin::EI_TASKBAR_WND_VALUE_RIGHT_ALIGN, std::to_wstring(m_taskbar_data.value_right_align).c_str());
            plugin_info.plugin->OnExtenedInfo(ITMPlugin::EI_TASKBAR_WND_NET_SPEED_WIDTH, std::to_wstring(m_taskbar_data.digits_number).c_str());
            plugin_info.plugin->OnExtenedInfo(ITMPlugin::EI_TASKBAR_WND_UNIT_BYTE, std::to_wstring(m_taskbar_data.unit_byte).c_str());
            plugin_info.plugin->OnExtenedInfo(ITMPlugin::EI_TASKBAR_WND_UNIT_SELECT, std::to_wstring(static_cast<int>(m_taskbar_data.speed_unit)).c_str());
            plugin_info.plugin->OnExtenedInfo(ITMPlugin::EI_TASKBAR_WND_NOT_SHOW_UNIT, std::to_wstring(m_taskbar_data.hide_unit).c_str());
            plugin_info.plugin->OnExtenedInfo(ITMPlugin::EI_TASKBAR_WND_NOT_SHOW_PERCENT, std::to_wstring(m_taskbar_data.hide_percent).c_str());
        }
    }
}

void CTrafficMonitorApp::UpdatePluginMenu(CMenu* pMenu, ITMPlugin* plugin, int plugin_cmd_start_index)
{
    if (pMenu != nullptr)
    {
        //Delete the existing plug-in command in the menu
        while (pMenu->GetMenuItemCount() > plugin_cmd_start_index)
        {
            if (!pMenu->DeleteMenu(pMenu->GetMenuItemCount() - 1, MF_BYPOSITION))
                break;
        }
        //Add plugin command
        if (plugin != nullptr && plugin->GetAPIVersion() >= 5)
        {
            int plugin_cmd_count = plugin->GetCommandCount();
            //Add separator
            if (plugin_cmd_count > 0)
                pMenu->AppendMenu(MF_SEPARATOR);
            for (int i = 0; i < plugin_cmd_count; i++)
            {
                const wchar_t* cmd_name = plugin->GetCommandName(i);
                if (cmd_name != nullptr)
                {
                    pMenu->AppendMenu(MF_STRING | MF_ENABLED, ID_PLUGIN_COMMAND_START + i, cmd_name);
                    //Add command icon
                    HICON cmd_icon = (HICON)plugin->GetCommandIcon(i);
                    CMenuIcon::AddIconToMenuItem(pMenu->GetSafeHmenu(), ID_PLUGIN_COMMAND_START + i, FALSE, cmd_icon);
                }
            }
        }
    }
}

void CTrafficMonitorApp::CheckWindows11Taskbar()
{
    // If you find a window with the class name "Windows.UI.Composition.DesktopWindowContentBridge" in the sub-window of "Shell_TrayWnd", it is considered to be the Windows 11 taskbar.
    if (m_win_version.IsWindows11OrLater())
    {
        HWND hTaskbar = ::FindWindow(L"Shell_TrayWnd", NULL);
        m_is_windows11_taskbar = (::FindWindowExW(hTaskbar, 0, L"Windows.UI.Composition.DesktopWindowContentBridge", NULL) != NULL);
    }
    else
    {
        m_is_windows11_taskbar = false;
    }
}

bool CTrafficMonitorApp::DPIFromRect(const RECT& rect, UINT* out_dpi_x, UINT* out_dpi_y)
{
    HMONITOR h_current_monitor = ::MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
    HRESULT hr = m_dll_functions.GetDpiForMonitor(h_current_monitor, MDT_EFFECTIVE_DPI, out_dpi_x, out_dpi_y);
    return hr == S_OK;
}

unsigned int CTrafficMonitorApp::GetThemeColor() const
{
    return m_theme_color;
}

void CTrafficMonitorApp::SetThemeColor(COLORREF color)
{
    m_theme_color = color;
}

void CTrafficMonitorApp::OnHelp()
{
    // TODO: Add command handler code here
    CString help_url;
    if (m_str_table.IsSimplifiedChinese())
        help_url = _T("https://github.com/zhongyang219/TrafficMonitor/wiki");
    else
        help_url = _T("https://github.com/zhongyang219/TrafficMonitor/wiki/Home_en");
    ShellExecute(NULL, _T("open"), help_url, NULL, NULL, SW_SHOW);
}


void CTrafficMonitorApp::OnFrequentyAskedQuestions()
{
    // TODO: Add command handler code here
    CString url_domain;
    if (static_cast<CUpdateHelper::UpdateSource>(m_general_data.update_source) == CUpdateHelper::UpdateSource::GiteeSource)
        url_domain = _T("gitee.com");
    else
        url_domain = _T("github.com");
    CString file_name;
    if (m_str_table.IsSimplifiedChinese())
        file_name = _T("Help.md");
    else
        file_name = _T("Help_en-us.md");
    CString url;
    url.Format(_T("https://%s/zhongyang219/TrafficMonitor/blob/master/%s"), url_domain.GetString(), file_name.GetString());
    ShellExecute(NULL, _T("open"), url, NULL, NULL, SW_SHOW);
}


void CTrafficMonitorApp::OnUpdateLog()
{
    // TODO: Add command handler code here
    CString url_domain;
    if (static_cast<CUpdateHelper::UpdateSource>(m_general_data.update_source) == CUpdateHelper::UpdateSource::GiteeSource)
        url_domain = _T("gitee.com");
    else
        url_domain = _T("github.com");
    wstring language_tag = m_str_table.GetLanguageInfo().bcp_47;
    CString file_name;
    if (language_tag == L"zh-CN")
        file_name = _T("update_log.md");
    else if (language_tag == L"zh-TW")
        file_name = _T("update_log_zh-tw.md");
    else
        file_name = _T("update_log_en-us.md");
    CString url;
    url.Format(_T("https://%s/zhongyang219/TrafficMonitor/blob/master/UpdateLog/%s"), url_domain.GetString(), file_name.GetString());
    ShellExecute(NULL, _T("open"), url, NULL, NULL, SW_SHOW);
}


int CTrafficMonitorApp::ExitInstance()
{
    // Release GDI+
    Gdiplus::GdiplusShutdown(m_gdiplusToken);

    return CWinApp::ExitInstance();
}

int CTrafficMonitorApp::GetAPIVersion()
{
    return 1;
}

const wchar_t* CTrafficMonitorApp::GetVersion()
{
    return VERSION;
}

double CTrafficMonitorApp::GetMonitorValue(MonitorItem item)
{
    switch (item)
    {
    case MI_UP: return m_out_speed;
    case MI_DOWN: return m_in_speed;
    case MI_CPU: return m_cpu_usage;
    case MI_MEMORY: return m_memory_usage;
    case MI_GPU_USAGE: return m_gpu_usage;
    case MI_CPU_TEMP: return m_cpu_temperature;
    case MI_GPU_TEMP: return m_gpu_temperature;
    case MI_HDD_TEMP: return m_hdd_temperature;
    case MI_MAIN_BOARD_TEMP: return m_main_board_temperature;
    case MI_HDD_USAGE: return m_hdd_usage;
    case MI_CPU_FREQ: return m_cpu_freq;
    case MI_TODAY_UP_TRAFFIC: return m_today_up_traffic;
    case MI_TODAY_DOWN_TRAFFIC: return m_today_down_traffic;
    }
    return 0.0;
}

const wchar_t* CTrafficMonitorApp::GetMonitorValueString(MonitorItem item, int is_main_window)
{
    static CString str_value;
    if (item == MI_TODAY_UP_TRAFFIC)
    {
        str_value = CCommon::KBytesToString(theApp.m_today_up_traffic / 1024u);
    }
    else if (item == MI_TODAY_DOWN_TRAFFIC)
    {
        str_value = CCommon::KBytesToString(theApp.m_today_down_traffic / 1024u);
    }
    else
    {
        DisplayItem display_item{};
        switch (item)
        {
        case MI_UP: display_item = DisplayItem::TDI_UP; break;
        case MI_DOWN: display_item = DisplayItem::TDI_DOWN; break;
        case MI_CPU: display_item = DisplayItem::TDI_CPU; break;
        case MI_MEMORY: display_item = DisplayItem::TDI_MEMORY; break;
        case MI_GPU_USAGE: display_item = DisplayItem::TDI_GPU_USAGE; break;
        case MI_CPU_TEMP: display_item = DisplayItem::TDI_CPU_TEMP; break;
        case MI_GPU_TEMP: display_item = DisplayItem::TDI_GPU_TEMP; break;
        case MI_HDD_TEMP: display_item = DisplayItem::TDI_HDD_TEMP; break;
        case MI_MAIN_BOARD_TEMP: display_item = DisplayItem::TDI_MAIN_BOARD_TEMP; break;
        case MI_HDD_USAGE: display_item = DisplayItem::TDI_HDD_USAGE; break;
        case MI_CPU_FREQ: display_item = DisplayItem::TDI_CPU_FREQ; break;
        }
        str_value = CommonDisplayItem(display_item).GetItemValueText(is_main_window);
    }
    return str_value.GetString();
}

void CTrafficMonitorApp::ShowNotifyMessage(const wchar_t* strMsg)
{
    CTrafficMonitorDlg* pMainWnd = dynamic_cast<CTrafficMonitorDlg*>(m_pMainWnd);
    if (pMainWnd != nullptr)
    {
        pMainWnd->ShowNotifyTip(CCommon::LoadText(IDS_TRAFFICMONITOR_PLUGIN_NITIFICATION), strMsg);
    }
}

unsigned short CTrafficMonitorApp::GetLanguageId() const
{
    return m_general_data.language.language_id;
}

const wchar_t* CTrafficMonitorApp::GetPluginConfigDir() const
{
    static std::wstring config_dir;
    config_dir = m_config_dir;
    config_dir += L"plugins\\";
    return config_dir.c_str();
}

int CTrafficMonitorApp::GetDPI(DPIType type) const
{
    CTrafficMonitorDlg* pMainWnd = CTrafficMonitorDlg::Instance();
    switch (type)
    {
    case DPI_MAIN_WND:
        return m_dpi;
    case DPI_TASKBAR:
        if (pMainWnd != nullptr)
        {
            if (pMainWnd->IsTaskbarWndValid())
                return pMainWnd->GetTaskbarWindow()->GetDPI();
        }
        return m_dpi;
    }
    return 0;
}

void* CTrafficMonitorApp::GetMainWindowHwnd()
{
    return m_pMainWnd->GetSafeHwnd();
}

void* CTrafficMonitorApp::GetTaskbarWindowHwnd()
{
    return CTrafficMonitorDlg::Instance()->GetTaskbarWindow()->GetSafeHwnd();
}

const wchar_t* CTrafficMonitorApp::GetStringRes(const wchar_t* key, const wchar_t* section)
{
    return m_str_table.LoadText(key, section).c_str();
}
