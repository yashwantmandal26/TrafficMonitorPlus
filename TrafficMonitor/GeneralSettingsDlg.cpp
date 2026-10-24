// GeneralSettingsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "TrafficMonitorDlg.h"
#include "GeneralSettingsDlg.h"
#include "PluginManagerDlg.h"
#include "SelectConnectionsDlg.h"


// CGeneralSettingsDlg dialog

static const int MONITOR_SPAN_STEP = 100;

IMPLEMENT_DYNAMIC(CGeneralSettingsDlg, CTabDlg)

CGeneralSettingsDlg::CGeneralSettingsDlg(CWnd* pParent /*=NULL*/)
    : CTabDlg(IDD_GENERAL_SETTINGS_DIALOG, pParent)
{

}

CGeneralSettingsDlg::~CGeneralSettingsDlg()
{
}

void CGeneralSettingsDlg::CheckTaskbarDisplayItem()
{
    //If a certain hardware monitoring is turned off in the option settings, the corresponding temperature monitoring related items will not be displayed.
    if (!theApp.m_general_data.IsHardwareEnable(HI_CPU))
    {
        theApp.m_taskbar_data.display_item.Remove(TDI_CPU_TEMP);
    }
    if (!theApp.m_general_data.IsHardwareEnable(HI_GPU))
    {
        theApp.m_taskbar_data.display_item.Remove(TDI_GPU_TEMP);
    }
    if (!theApp.m_general_data.IsHardwareEnable(HI_HDD))
    {
        theApp.m_taskbar_data.display_item.Remove(TDI_HDD_TEMP);
    }
    if (!theApp.m_general_data.IsHardwareEnable(HI_MBD))
        theApp.m_taskbar_data.display_item.Remove(TDI_MAIN_BOARD_TEMP);
}

void CGeneralSettingsDlg::SetControlMouseWheelEnable(bool enable)
{
    m_traffic_tip_combo.SetMouseWheelEnable(enable);
    m_language_combo.SetMouseWheelEnable(enable);
    m_traffic_tip_edit.SetMouseWheelEnable(enable);
    m_memory_tip_edit.SetMouseWheelEnable(enable);
    m_monitor_span_edit.SetMouseWheelEnable(enable);
    m_cpu_temp_tip_edit.SetMouseWheelEnable(enable);
    m_gpu_temp_tip_edit.SetMouseWheelEnable(enable);
    m_hdd_temp_tip_edit.SetMouseWheelEnable(enable);
    m_mbd_temp_tip_edit.SetMouseWheelEnable(enable);
    m_hard_disk_combo.SetMouseWheelEnable(enable);
    m_select_cpu_combo.SetMouseWheelEnable(enable);
}

void CGeneralSettingsDlg::OnSettingsApplied()
{
    //When the setting is applied, reset the value of xxxx_ori
    m_monitor_time_span_ori = m_data.monitor_time_span;
    m_update_source_ori = m_data.update_source;
}

bool CGeneralSettingsDlg::InitializeControls()
{
    RepositionTextBasedControls({
        { CtrlTextInfo::L4, IDC_CHECK_NOW_BUTTON, CtrlTextInfo::W16 }
        });
    RepositionTextBasedControls({
        { CtrlTextInfo::L4, IDC_UPDATE_SORUCE_STATIC },
        { CtrlTextInfo::L3, IDC_GITHUB_RADIO },
        { CtrlTextInfo::L2, IDC_GITEE_RADIO }
        });
    RepositionTextBasedControls({
        { CtrlTextInfo::L4, IDC_RESET_AUTO_RUN_BUTTON, CtrlTextInfo::W16 }
        });

    RepositionTextBasedControls({
        { CtrlTextInfo::L4, IDC_LANGUAGE_STATIC },
        { CtrlTextInfo::L3, IDC_LANGUAGE_COMBO }
        });

    //Adjust the horizontal position of the row control "Today's usage traffic has reached"
    RepositionTextBasedControls({
        { CtrlTextInfo::L4, IDC_TODAY_TRAFFIC_TIP_CHECK, CtrlTextInfo::W24 },
        { CtrlTextInfo::L3, IDC_TODAY_TRAFFIC_TIP_EDIT },
        { CtrlTextInfo::L2, IDC_TODAY_TRAFFIC_TIP_COMBO },
        { CtrlTextInfo::L1, IDC_TODAY_TRAFFIC_BACK_STATIC}
        });
    //Adjust the horizontal position of the controls in the lines "Memory Usage Has Reached" and "Temperature Has Reached"
    RepositionTextBasedControls({
        { CtrlTextInfo::L4, IDC_MEMORY_USAGE_TIP_CHECK, CtrlTextInfo::W24 },
        { CtrlTextInfo::L3, IDC_MEMORY_USAGE_TIP_EDIT },
        { CtrlTextInfo::L2, IDC_MEMORY_USAGE_BACK_STATIC },
        { CtrlTextInfo::L4, IDC_CPU_TEMP_TIP_CHECK, CtrlTextInfo::W24 },
        { CtrlTextInfo::L3, IDC_CPU_TEMP_TIP_EDIT },
        { CtrlTextInfo::L2, IDC_CPU_TEMP_STATIC },
        { CtrlTextInfo::L4, IDC_GPU_TEMP_TIP_CHECK, CtrlTextInfo::W24 },
        { CtrlTextInfo::L3, IDC_GPU_TEMP_TIP_EDIT },
        { CtrlTextInfo::L2, IDC_GPU_TEMP_STATIC },
        { CtrlTextInfo::L4, IDC_HDD_TEMP_TIP_CHECK, CtrlTextInfo::W24 },
        { CtrlTextInfo::L3, IDC_HDD_TIP_EDIT },
        { CtrlTextInfo::L2, IDC_HDD_STATIC },
        { CtrlTextInfo::L4, IDC_MBD_TEMP_TIP_CHECK, CtrlTextInfo::W24 },
        { CtrlTextInfo::L3, IDC_MBD_TEMP_TIP_EDIT },
        { CtrlTextInfo::L2, IDC_MBD_TEMP_STATIC },
        });
    RepositionTextBasedControls({
        { CtrlTextInfo::L1, IDC_SELECT_HDD_STATIC },
        { CtrlTextInfo::C0, IDC_SELECT_HARD_DISK_COMBO },
        { CtrlTextInfo::L1, IDC_SELECT_CPU_STATIC },
        { CtrlTextInfo::C0, IDC_SELECT_CPU_COMBO },
    });

    RepositionTextBasedControls({
        { CtrlTextInfo::L4, IDC_SELECT_CONNECTIONS_BUTTON, CtrlTextInfo::W32 }
    });
    RepositionTextBasedControls({
        { CtrlTextInfo::L4, IDC_MONITOR_INTERVAL_STATIC },
        { CtrlTextInfo::L3, IDC_MONITOR_SPAN_EDIT },
        { CtrlTextInfo::L2, IDC_MILLISECONDS_STATIC },
        { CtrlTextInfo::L1, IDC_RESTORE_DEFAULT_TIME_SPAN_BUTTON, CtrlTextInfo::W16 }
    });
    RepositionTextBasedControls({
        { CtrlTextInfo::L4, IDC_PLUGIN_MANAGE_BUTTON, CtrlTextInfo::W32 }
    });

    return true;
}

bool CGeneralSettingsDlg::ShowHardwareMonitorWarning()
{
    //If there is already a hardware monitoring item checked, no prompt will pop up again.
    if (m_data.hardware_monitor_item != 0)
        return true;

    if (SHMessageBoxCheck(m_hWnd, CCommon::LoadText(IDS_HARDWARE_MONITOR_WARNING), APP_NAME, MB_OKCANCEL | MB_ICONWARNING, IDOK, _T("{B8A281A7-76DF-4F0F-BF6A-1A394EF8BAD5}")) == IDOK)
        return true;

    return false;
}

void CGeneralSettingsDlg::AddOrUpdateAutoRunTooltip(bool add)
{
    if (m_data.auto_run)
    {
        CString str_tool_tip;
        if (m_data.auto_run_by_task_scheduler)
            str_tool_tip = CCommon::LoadText(IDS_AUTO_RUN_METHOD_TASK_SCHEDULE);
        else
            str_tool_tip = CCommon::LoadText(IDS_AUTO_RUN_METHOD_REGESTRY);
        if (!m_auto_run_path.empty())
        {
            str_tool_tip += _T("\r\n");
            str_tool_tip += CCommon::LoadText(IDS_PATH, _T(": "));
            str_tool_tip += m_auto_run_path.c_str();
        }
        if (add)
            m_toolTip.AddTool(GetDlgItem(IDC_AUTO_RUN_CHECK), str_tool_tip);
        else
            m_toolTip.UpdateTipText(str_tool_tip, GetDlgItem(IDC_AUTO_RUN_CHECK));
    }
    else
    {
        if (add)
            m_toolTip.AddTool(GetDlgItem(IDC_AUTO_RUN_CHECK), CString());
        else
            m_toolTip.UpdateTipText(CString(), GetDlgItem(IDC_AUTO_RUN_CHECK));
    }
}

bool CGeneralSettingsDlg::IsMonitorTimeSpanModified() const
{
    return m_data.monitor_time_span != m_monitor_time_span_ori;
}

void CGeneralSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
    CTabDlg::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TODAY_TRAFFIC_TIP_EDIT, m_traffic_tip_edit);
    DDX_Control(pDX, IDC_TODAY_TRAFFIC_TIP_COMBO, m_traffic_tip_combo);
    DDX_Control(pDX, IDC_MEMORY_USAGE_TIP_EDIT, m_memory_tip_edit);
    DDX_Control(pDX, IDC_LANGUAGE_COMBO, m_language_combo);
    DDX_Control(pDX, IDC_MONITOR_SPAN_EDIT, m_monitor_span_edit);
    DDX_Control(pDX, IDC_CPU_TEMP_TIP_EDIT, m_cpu_temp_tip_edit);
    DDX_Control(pDX, IDC_GPU_TEMP_TIP_EDIT, m_gpu_temp_tip_edit);
    DDX_Control(pDX, IDC_HDD_TIP_EDIT, m_hdd_temp_tip_edit);
    DDX_Control(pDX, IDC_MBD_TEMP_TIP_EDIT, m_mbd_temp_tip_edit);
    DDX_Control(pDX, IDC_SELECT_HARD_DISK_COMBO, m_hard_disk_combo);
    DDX_Control(pDX, IDC_SELECT_CPU_COMBO, m_select_cpu_combo);
    DDX_Control(pDX, IDC_PLUGIN_MANAGE_BUTTON, m_plugin_manager_btn);
    DDX_Control(pDX, IDC_SELECT_CONNECTIONS_BUTTON, m_select_connection_btn);
}

void CGeneralSettingsDlg::SetControlEnable()
{
    m_traffic_tip_edit.EnableWindow(m_data.traffic_tip_enable);
    m_traffic_tip_combo.EnableWindow(m_data.traffic_tip_enable);
    m_memory_tip_edit.EnableWindow(m_data.memory_usage_tip.enable);
    m_cpu_temp_tip_edit.EnableWindow(m_data.cpu_temp_tip.enable);
    m_gpu_temp_tip_edit.EnableWindow(m_data.gpu_temp_tip.enable);
    m_hdd_temp_tip_edit.EnableWindow(m_data.hdd_temp_tip.enable);
    m_mbd_temp_tip_edit.EnableWindow(m_data.mainboard_temp_tip.enable);

    //m_hard_disk_combo.EnableWindow(m_data.IsHardwareEnable(HI_HDD));
    m_select_cpu_combo.EnableWindow(m_data.IsHardwareEnable(HI_CPU));

    EnableDlgCtrl(IDC_SELECT_CONNECTIONS_BUTTON, !m_data.show_all_interface);

    EnableDlgCtrl(IDC_RESET_AUTO_RUN_BUTTON, m_data.auto_run);
    EnableDlgCtrl(IDC_AUTO_RUN_METHOD_REGESTRY_RADIO, m_data.auto_run);
    EnableDlgCtrl(IDC_AUTO_RUN_METHOD_TASK_SCHEDULE_RADIO, m_data.auto_run);
}


BEGIN_MESSAGE_MAP(CGeneralSettingsDlg, CTabDlg)
    ON_BN_CLICKED(IDC_CHECK_NOW_BUTTON, &CGeneralSettingsDlg::OnBnClickedCheckNowButton)
    ON_BN_CLICKED(IDC_CHECK_UPDATE_CHECK, &CGeneralSettingsDlg::OnBnClickedCheckUpdateCheck)
    ON_BN_CLICKED(IDC_AUTO_RUN_CHECK, &CGeneralSettingsDlg::OnBnClickedAutoRunCheck)
    ON_BN_CLICKED(IDC_TODAY_TRAFFIC_TIP_CHECK, &CGeneralSettingsDlg::OnBnClickedTodayTrafficTipCheck)
    ON_BN_CLICKED(IDC_MEMORY_USAGE_TIP_CHECK, &CGeneralSettingsDlg::OnBnClickedMemoryUsageTipCheck)
    ON_BN_CLICKED(IDC_OPEN_CONFIG_PATH_BUTTON, &CGeneralSettingsDlg::OnBnClickedOpenConfigPathButton)
    ON_BN_CLICKED(IDC_SHOW_ALL_CONNECTION_CHECK, &CGeneralSettingsDlg::OnBnClickedShowAllConnectionCheck)
    ON_BN_CLICKED(IDC_USE_CPU_TIME_RADIO, &CGeneralSettingsDlg::OnBnClickedUseCpuTimeRadio)
    ON_BN_CLICKED(IDC_USE_PDH_RADIO, &CGeneralSettingsDlg::OnBnClickedUsePdhRadio)
    ON_EN_KILLFOCUS(IDC_MONITOR_SPAN_EDIT, &CGeneralSettingsDlg::OnEnKillfocusMonitorSpanEdit)
    ON_BN_CLICKED(IDC_CPU_TEMP_TIP_CHECK, &CGeneralSettingsDlg::OnBnClickedCpuTempTipCheck)
    ON_BN_CLICKED(IDC_GPU_TEMP_TIP_CHECK, &CGeneralSettingsDlg::OnBnClickedGpuTempTipCheck)
    ON_BN_CLICKED(IDC_HDD_TEMP_TIP_CHECK, &CGeneralSettingsDlg::OnBnClickedHddTempTipCheck)
    ON_BN_CLICKED(IDC_MBD_TEMP_TIP_CHECK, &CGeneralSettingsDlg::OnBnClickedMbdTempTipCheck)
    ON_BN_CLICKED(IDC_GITHUB_RADIO, &CGeneralSettingsDlg::OnBnClickedGithubRadio)
    ON_BN_CLICKED(IDC_GITEE_RADIO, &CGeneralSettingsDlg::OnBnClickedGiteeRadio)
    ON_BN_CLICKED(IDC_RESTORE_DEFAULT_TIME_SPAN_BUTTON, &CGeneralSettingsDlg::OnBnClickedRestoreDefaultTimeSpanButton)
    ON_CBN_SELCHANGE(IDC_SELECT_HARD_DISK_COMBO, &CGeneralSettingsDlg::OnCbnSelchangeSelectHardDiskCombo)
    ON_BN_CLICKED(IDC_CPU_CHECK, &CGeneralSettingsDlg::OnBnClickedCpuCheck)
    ON_BN_CLICKED(IDC_GPU_CHECK, &CGeneralSettingsDlg::OnBnClickedGpuCheck)
    ON_BN_CLICKED(IDC_HDD_CHECK, &CGeneralSettingsDlg::OnBnClickedHddCheck)
    ON_BN_CLICKED(IDC_MBD_CHECK, &CGeneralSettingsDlg::OnBnClickedMbdCheck)
    ON_CBN_SELCHANGE(IDC_SELECT_CPU_COMBO, &CGeneralSettingsDlg::OnCbnSelchangeSelectCpuCombo)
    ON_BN_CLICKED(IDC_PLUGIN_MANAGE_BUTTON, &CGeneralSettingsDlg::OnBnClickedPluginManageButton)
    ON_BN_CLICKED(IDC_SHOW_NOTIFY_ICON_CHECK, &CGeneralSettingsDlg::OnBnClickedShowNotifyIconCheck)
    ON_BN_CLICKED(IDC_SELECT_CONNECTIONS_BUTTON, &CGeneralSettingsDlg::OnBnClickedSelectConnectionsButton)
    ON_BN_CLICKED(IDC_RESET_AUTO_RUN_BUTTON, &CGeneralSettingsDlg::OnBnClickedResetAutoRunButton)
    ON_EN_CHANGE(IDC_MONITOR_SPAN_EDIT, &CGeneralSettingsDlg::OnEnChangeMonitorSpanEdit)
    ON_MESSAGE(WM_SPIN_EDIT_POS_CHANGED, &CGeneralSettingsDlg::OnSpinEditPosChanged)
    ON_BN_CLICKED(IDC_AUTO_RUN_METHOD_REGESTRY_RADIO, &CGeneralSettingsDlg::OnBnClickedAutoRunMethodRegestryRadio)
    ON_BN_CLICKED(IDC_AUTO_RUN_METHOD_TASK_SCHEDULE_RADIO, &CGeneralSettingsDlg::OnBnClickedAutoRunMethodTaskScheduleRadio)
END_MESSAGE_MAP()


// CGeneralSettingsDlg message handler


BOOL CGeneralSettingsDlg::OnInitDialog()
{
    CTabDlg::OnInitDialog();

    // TODO: Add additional initialization here

    ((CButton*)GetDlgItem(IDC_CHECK_UPDATE_CHECK))->SetCheck(m_data.check_update_when_start);
    if (theApp.IsForceShowNotifyIcon())
    {
        m_data.show_notify_icon = true;
        EnableDlgCtrl(IDC_SHOW_NOTIFY_ICON_CHECK, FALSE);
    }
    CheckDlgButton(IDC_SHOW_NOTIFY_ICON_CHECK, m_data.show_notify_icon);

    if (m_data.update_source == 0)
        CheckDlgButton(IDC_GITHUB_RADIO, TRUE);
    else
        CheckDlgButton(IDC_GITEE_RADIO, TRUE);

    //Check whether there is a shortcut to the program in the "Startup" directory of the start menu. If so, set it to start automatically at boot, and then delete the shortcut.
    wstring start_up_path = CCommon::GetStartUpPath();
    bool shortcut_exist = CCommon::FileExist((start_up_path + L"\\TrafficMonitor.lnk").c_str());
    m_data.auto_run = false;
#ifdef WITHOUT_TEMPERATURE
    m_data.auto_run_by_task_scheduler = false;
#else
    m_data.auto_run_by_task_scheduler = true;
#endif

    if (shortcut_exist)
    {
        theApp.SetAutoRun(true, false);
        m_data.auto_run = true;
        DeleteFile((start_up_path + L"\\TrafficMonitor.lnk").c_str());
        CheckDlgButton(IDC_AUTO_RUN_METHOD_REGESTRY_RADIO, TRUE);
    }
    //Check whether the setting to run automatically at startup is implemented through the registry or task scheduler, and set the corresponding options
    else
    {
        bool auto_run_by_registry = theApp.GetAutoRun(&m_auto_run_path, false);
        if (auto_run_by_registry)
        {
            CheckDlgButton(IDC_AUTO_RUN_METHOD_REGESTRY_RADIO, TRUE);
            m_data.auto_run = true;
            m_data.auto_run_by_task_scheduler = false;
        }
        else
        {
            bool auto_run_by_task_scheduler = theApp.GetAutoRun(&m_auto_run_path, true);
            if (auto_run_by_task_scheduler)
            {
                CheckDlgButton(IDC_AUTO_RUN_METHOD_TASK_SCHEDULE_RADIO, TRUE);
                m_data.auto_run = true;
                m_data.auto_run_by_task_scheduler = true;
            }
        }
    }
    if (m_data.auto_run_by_task_scheduler)
        CheckDlgButton(IDC_AUTO_RUN_METHOD_TASK_SCHEDULE_RADIO, TRUE);
    else
        CheckDlgButton(IDC_AUTO_RUN_METHOD_REGESTRY_RADIO, TRUE);


    ((CButton*)GetDlgItem(IDC_SAVE_TO_APPDATA_RADIO))->SetCheck(!m_data.portable_mode);
    ((CButton*)GetDlgItem(IDC_SAVE_TO_PROGRAM_DIR_RADIO))->SetCheck(m_data.portable_mode);
    GetDlgItem(IDC_SAVE_TO_PROGRAM_DIR_RADIO)->EnableWindow(theApp.m_module_dir_writable);

    ((CButton*)GetDlgItem(IDC_AUTO_RUN_CHECK))->SetCheck(m_data.auto_run);

    ((CButton*)GetDlgItem(IDC_TODAY_TRAFFIC_TIP_CHECK))->SetCheck(m_data.traffic_tip_enable);
    m_traffic_tip_edit.SetRange(1, 32767);
    m_traffic_tip_edit.SetValue(m_data.traffic_tip_value);
    m_traffic_tip_combo.AddString(_T("MB"));
    m_traffic_tip_combo.AddString(_T("GB"));
    m_traffic_tip_combo.SetCurSel(m_data.traffic_tip_unit);
    CheckDlgButton(IDC_MEMORY_USAGE_TIP_CHECK, m_data.memory_usage_tip.enable);
    m_memory_tip_edit.SetRange(1, 100);
    m_memory_tip_edit.SetValue(m_data.memory_usage_tip.tip_value);

    CheckDlgButton(IDC_CPU_TEMP_TIP_CHECK, m_data.cpu_temp_tip.enable);
    m_cpu_temp_tip_edit.SetRange(1, 120);
    m_cpu_temp_tip_edit.SetValue(m_data.cpu_temp_tip.tip_value);

    CheckDlgButton(IDC_GPU_TEMP_TIP_CHECK, m_data.gpu_temp_tip.enable);
    m_gpu_temp_tip_edit.SetRange(1, 120);
    m_gpu_temp_tip_edit.SetValue(m_data.gpu_temp_tip.tip_value);

    CheckDlgButton(IDC_HDD_TEMP_TIP_CHECK, m_data.hdd_temp_tip.enable);
    m_hdd_temp_tip_edit.SetRange(1, 120);
    m_hdd_temp_tip_edit.SetValue(m_data.hdd_temp_tip.tip_value);

    CheckDlgButton(IDC_MBD_TEMP_TIP_CHECK, m_data.mainboard_temp_tip.enable);
    m_mbd_temp_tip_edit.SetRange(1, 120);
    m_mbd_temp_tip_edit.SetValue(m_data.mainboard_temp_tip.tip_value);

    SetControlEnable();

    m_language_combo.AddString(CCommon::LoadText(IDS_FOLLOWING_SYSTEM));
    int current_language_index{ -1 };       //The serial number of the current language in the list of all languages
    for (size_t i = 0; i < theApp.m_str_table.GetLanguageList().size(); i++)
    {
        const LanguageInfo& language_info = theApp.m_str_table.GetLanguageList()[i];
        m_language_combo.AddString(language_info.display_name.c_str());
        if (language_info == m_data.language)
            current_language_index = static_cast<int>(i);
    }
    m_language_combo.SetCurSel(current_language_index + 1);     //Since the first item of ComboBox is "following the system", the serial number of ComboBox needs to be increased by 1.

    ((CButton*)GetDlgItem(IDC_SHOW_ALL_CONNECTION_CHECK))->SetCheck(m_data.show_all_interface);

    m_toolTip.Create(this);
    m_toolTip.SetMaxTipWidth(theApp.DPI(300));
    m_toolTip.AddTool(GetDlgItem(IDC_SHOW_ALL_CONNECTION_CHECK), CCommon::LoadText(IDS_SHOW_ALL_INFO_TIP));
    m_toolTip.AddTool(GetDlgItem(IDC_SAVE_TO_APPDATA_RADIO), theApp.m_appdata_dir.c_str());
    m_toolTip.AddTool(GetDlgItem(IDC_SAVE_TO_PROGRAM_DIR_RADIO), theApp.m_module_dir.c_str());
    AddOrUpdateAutoRunTooltip(true);

    if (m_data.cpu_usage_acquire_method == GeneralSettingData::CA_CPU_TIME)
    {
        CheckDlgButton(IDC_USE_CPU_TIME_RADIO, TRUE);
    }
    else if (m_data.cpu_usage_acquire_method == GeneralSettingData::CA_PDH)
    {
        CheckDlgButton(IDC_USE_PDH_RADIO, TRUE);
    }

    m_monitor_span_edit.SetRange(MONITOR_TIME_SPAN_MIN, MONITOR_TIME_SPAN_MAX, MONITOR_SPAN_STEP);
    m_monitor_span_edit.SetValue(m_data.monitor_time_span);

    m_monitor_time_span_ori = m_data.monitor_time_span;
    m_update_source_ori = m_data.update_source;

    if (CTrafficMonitorDlg::Instance()->IsGetDiskUsageByPdh())
    {
        const auto& disk_names = CTrafficMonitorDlg::Instance()->GetPdhDiskUsageHelper().GetDiskNames();
        for (const auto& hdd_name : disk_names)
            m_hard_disk_combo.AddString(hdd_name);
        int cur_index = m_hard_disk_combo.FindString(-1, m_data.hard_disk_name.c_str());
        m_hard_disk_combo.SetCurSel(cur_index);
    }
#ifndef WITHOUT_TEMPERATURE
    //Initialize the hardware monitoring check box
    CheckDlgButton(IDC_CPU_CHECK, m_data.IsHardwareEnable(HI_CPU));
    CheckDlgButton(IDC_GPU_CHECK, m_data.IsHardwareEnable(HI_GPU));
    CheckDlgButton(IDC_HDD_CHECK, m_data.IsHardwareEnable(HI_HDD));
    CheckDlgButton(IDC_MBD_CHECK, m_data.IsHardwareEnable(HI_MBD));

    if (theApp.m_pMonitor != nullptr)
    {
        CSingleLock sync(&theApp.m_minitor_lib_critical, TRUE);
        //Initialize the hard disk selection drop-down list
        if (!CTrafficMonitorDlg::Instance()->IsGetDiskUsageByPdh())
        {
            for (const auto& hdd_item : theApp.m_pMonitor->AllHDDTemperature())
                m_hard_disk_combo.AddString(hdd_item.first.c_str());
            int cur_index = m_hard_disk_combo.FindString(-1, m_data.hard_disk_name.c_str());
            m_hard_disk_combo.SetCurSel(cur_index);
        }
        //Initialize select CPU drop-down list
        m_select_cpu_combo.AddString(CCommon::LoadText(IDS_AVREAGE_TEMPERATURE));
        for (const auto& cpu_item : theApp.m_pMonitor->AllCpuTemperature())
            m_select_cpu_combo.AddString(cpu_item.first.c_str());
        int cur_index = m_select_cpu_combo.FindString(-1, m_data.cpu_core_name.c_str());
        if (cur_index < 0)
            cur_index = 0;
        m_select_cpu_combo.SetCurSel(cur_index);
    }
#endif

    //Version without temperature monitoring, disable temperature-related controls
#ifdef WITHOUT_TEMPERATURE
    EnableDlgCtrl(IDC_CPU_TEMP_TIP_CHECK, false);
    EnableDlgCtrl(IDC_CPU_TEMP_TIP_EDIT, false);
    EnableDlgCtrl(IDC_GPU_TEMP_TIP_CHECK, false);
    EnableDlgCtrl(IDC_GPU_TEMP_TIP_EDIT, false);
    EnableDlgCtrl(IDC_HDD_TEMP_TIP_CHECK, false);
    EnableDlgCtrl(IDC_HDD_TIP_EDIT, false);
    EnableDlgCtrl(IDC_MBD_TEMP_TIP_CHECK, false);
    EnableDlgCtrl(IDC_MBD_TEMP_TIP_EDIT, false);
    EnableDlgCtrl(IDC_CPU_CHECK, false);
    EnableDlgCtrl(IDC_GPU_CHECK, false);
    EnableDlgCtrl(IDC_HDD_CHECK, false);
    EnableDlgCtrl(IDC_MBD_CHECK, false);
    //EnableDlgCtrl(IDC_SELECT_HARD_DISK_COMBO, false);
    EnableDlgCtrl(IDC_SELECT_CPU_COMBO, false);
    EnableDlgCtrl(IDC_CPU_TEMP_STATIC, false);
    EnableDlgCtrl(IDC_GPU_TEMP_STATIC, false);
    EnableDlgCtrl(IDC_HDD_STATIC, false);
    EnableDlgCtrl(IDC_MBD_TEMP_STATIC, false);
    //EnableDlgCtrl(IDC_SELECT_HDD_STATIC, false);
    EnableDlgCtrl(IDC_SELECT_CPU_STATIC, false);
    EnableDlgCtrl(IDC_HARDWARE_MONITOR_STATIC, false);
#endif

    m_plugin_manager_btn.SetIcon(theApp.GetMenuIcon(IDI_PLUGINS));
    m_select_connection_btn.SetIcon(theApp.GetMenuIcon(IDI_CONNECTION));

    return TRUE;  // return TRUE unless you set the focus to a control
                  // Exception: OCX property page should return FALSE
}


void CGeneralSettingsDlg::OnBnClickedCheckNowButton()
{
    // TODO: Add control notification handler code here
    theApp.CheckUpdateInThread(true);
}


void CGeneralSettingsDlg::OnBnClickedCheckUpdateCheck()
{
    // TODO: Add control notification handler code here
    m_data.check_update_when_start = (IsDlgButtonChecked(IDC_CHECK_UPDATE_CHECK) != 0);
}


void CGeneralSettingsDlg::OnBnClickedAutoRunCheck()
{
    // TODO: Add control notification handler code here
    m_data.auto_run = (IsDlgButtonChecked(IDC_AUTO_RUN_CHECK) != 0);
    m_auto_run_modified = true;
    SetControlEnable();
}


void CGeneralSettingsDlg::OnOK()
{
    // TODO: Add specialized code here and/or call the base class
    //Get message prompt settings
    m_data.traffic_tip_value = m_traffic_tip_edit.GetValue();
    if (m_data.traffic_tip_value < 1) m_data.traffic_tip_value = 1;
    if (m_data.traffic_tip_value > 32767) m_data.traffic_tip_value = 32767;
    m_data.traffic_tip_unit = m_traffic_tip_combo.GetCurSel();

    auto checkTipValue = [](int& value)
    {
        if (value < 1) value = 1;
        if (value > 100) value = 100;
    };
    auto checkTempTipValue = [](int& value)
    {
        if (value < 1) value = 1;
        if (value > 120) value = 120;
    };
    m_data.memory_usage_tip.tip_value = m_memory_tip_edit.GetValue();
    checkTipValue(m_data.memory_usage_tip.tip_value);

    m_data.cpu_temp_tip.tip_value = m_cpu_temp_tip_edit.GetValue();
    checkTempTipValue(m_data.cpu_temp_tip.tip_value);

    m_data.gpu_temp_tip.tip_value = m_gpu_temp_tip_edit.GetValue();
    checkTempTipValue(m_data.gpu_temp_tip.tip_value);

    m_data.hdd_temp_tip.tip_value = m_hdd_temp_tip_edit.GetValue();
    checkTempTipValue(m_data.hdd_temp_tip.tip_value);

    m_data.mainboard_temp_tip.tip_value = m_mbd_temp_tip_edit.GetValue();
    checkTempTipValue(m_data.mainboard_temp_tip.tip_value);

    //Get language settings
    m_data.language = LanguageInfo();
    if (m_language_combo.GetCurSel() > 0)
    {
        //The choice was not "Follow the system"
        int current_language_index = m_language_combo.GetCurSel() - 1;
        if (current_language_index >= 0 && current_language_index < static_cast<int>(theApp.m_str_table.GetLanguageList().size()))
        {
            m_data.language = theApp.m_str_table.GetLanguageList()[current_language_index];
        }
    }
    if (m_data.language != theApp.m_general_data.language)
    {
        MessageBox(CCommon::LoadText(IDS_LANGUAGE_CHANGE_INFO), NULL, MB_ICONINFORMATION | MB_OK);
    }
    m_show_all_interface_modified = (m_data.show_all_interface != theApp.m_general_data.show_all_interface);

    //Get the settings for where the data files are saved
    m_data.portable_mode = (((CButton*)GetDlgItem(IDC_SAVE_TO_PROGRAM_DIR_RADIO))->GetCheck() != 0);
    if (m_data.portable_mode != theApp.m_general_data.portable_mode)
    {
        MessageBox(CCommon::LoadText(IDS_CFG_DIR_CHANGED_INFO), NULL, MB_ICONINFORMATION | MB_OK);
    }

    //m_taskbar_item_modified = (theApp.m_taskbar_data.display_item != taskbar_displat_item_ori);

    CTabDlg::OnOK();
}


void CGeneralSettingsDlg::OnBnClickedTodayTrafficTipCheck()
{
    // TODO: Add control notification handler code here
    m_data.traffic_tip_enable = (IsDlgButtonChecked(IDC_TODAY_TRAFFIC_TIP_CHECK) != 0);
    SetControlEnable();
}


void CGeneralSettingsDlg::OnBnClickedMemoryUsageTipCheck()
{
    // TODO: Add control notification handler code here
    m_data.memory_usage_tip.enable = (IsDlgButtonChecked(IDC_MEMORY_USAGE_TIP_CHECK) != 0);
    SetControlEnable();
}


void CGeneralSettingsDlg::OnBnClickedOpenConfigPathButton()
{
    // TODO: Add control notification handler code here
    ShellExecute(NULL, _T("explore"), theApp.m_config_dir.c_str(), NULL, NULL, SW_SHOWNORMAL);
}


void CGeneralSettingsDlg::OnBnClickedShowAllConnectionCheck()
{
    // TODO: Add control notification handler code here
    m_data.show_all_interface = (IsDlgButtonChecked(IDC_SHOW_ALL_CONNECTION_CHECK) != 0);
    SetControlEnable();
}


BOOL CGeneralSettingsDlg::PreTranslateMessage(MSG* pMsg)
{
    // TODO: Add specialized code here and/or call the base class
    if (pMsg->message == WM_MOUSEMOVE)
        m_toolTip.RelayEvent(pMsg);

    return CTabDlg::PreTranslateMessage(pMsg);
}

void CGeneralSettingsDlg::OnBnClickedUseCpuTimeRadio()
{
    m_data.cpu_usage_acquire_method = GeneralSettingData::CA_CPU_TIME;
}


void CGeneralSettingsDlg::OnBnClickedUsePdhRadio()
{
    m_data.cpu_usage_acquire_method = GeneralSettingData::CA_PDH;
}

afx_msg LRESULT CGeneralSettingsDlg::OnSpinEditPosChanged(WPARAM wParam, LPARAM lParam)
{
    CSpinButtonCtrl* pSpin = (CSpinButtonCtrl*)wParam;
    if (pSpin == nullptr)
        return 0;
    CWnd* pEdit = pSpin->GetBuddy();
    if (pEdit == &m_monitor_span_edit)       //When the user clicks the fine-tuning button of "Monitoring Interval"
    {
        LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(lParam);
        if (pNMUpDown->iDelta == -1)
        {
            // The user pressed the down arrow of the spin control
            int value = m_monitor_span_edit.GetValue();
            value -= MONITOR_SPAN_STEP;
            value /= MONITOR_SPAN_STEP;
            value *= MONITOR_SPAN_STEP;
            m_monitor_span_edit.SetValue(value);
        }
        else if (pNMUpDown->iDelta == 1)
        {
            // The user pressed the up arrow of the spin control
            int value = m_monitor_span_edit.GetValue();
            value += MONITOR_SPAN_STEP;
            value /= MONITOR_SPAN_STEP;
            value *= MONITOR_SPAN_STEP;
            m_monitor_span_edit.SetValue(value);
        }
        pNMUpDown->iDelta = 0;
    }
    return 0;
}



void CGeneralSettingsDlg::OnEnKillfocusMonitorSpanEdit()
{
    // TODO: Add control notification handler code here

    //The monitoring time interval here is limited to multiples of 100.
    CString str;
    GetDlgItemText(IDC_MONITOR_SPAN_EDIT, str);
    str.Replace(_T(","), _T(""));
    int value = _ttoi(str.GetString());
    if (value < MONITOR_TIME_SPAN_MIN || value > MONITOR_TIME_SPAN_MAX)
    {
        value = 1000;
    }
    else
    {
        value /= MONITOR_SPAN_STEP;
        value *= MONITOR_SPAN_STEP;
    }
    m_monitor_span_edit.SetValue(value);
}


void CGeneralSettingsDlg::OnBnClickedCpuTempTipCheck()
{
    // TODO: Add control notification handler code here
    m_data.cpu_temp_tip.enable = (IsDlgButtonChecked(IDC_CPU_TEMP_TIP_CHECK) != 0);
    SetControlEnable();
}


void CGeneralSettingsDlg::OnBnClickedGpuTempTipCheck()
{
    // TODO: Add control notification handler code here
    m_data.gpu_temp_tip.enable = (IsDlgButtonChecked(IDC_GPU_TEMP_TIP_CHECK) != 0);
    SetControlEnable();
}


void CGeneralSettingsDlg::OnBnClickedHddTempTipCheck()
{
    // TODO: Add control notification handler code here
    m_data.hdd_temp_tip.enable = (IsDlgButtonChecked(IDC_HDD_TEMP_TIP_CHECK) != 0);
    SetControlEnable();
}


void CGeneralSettingsDlg::OnBnClickedMbdTempTipCheck()
{
    // TODO: Add control notification handler code here
    m_data.mainboard_temp_tip.enable = (IsDlgButtonChecked(IDC_MBD_TEMP_TIP_CHECK) != 0);
    SetControlEnable();
}


void CGeneralSettingsDlg::OnBnClickedGithubRadio()
{
    // TODO: Add control notification handler code here
    m_data.update_source = 0;
    theApp.m_general_data.update_source = 0;        //When clicking the radio button for "Update Source", also change the settings in theApp to ensure that the selected update source is used when clicking the "Check Now" button
}


void CGeneralSettingsDlg::OnBnClickedGiteeRadio()
{
    // TODO: Add control notification handler code here
    m_data.update_source = 1;
    theApp.m_general_data.update_source = 1;
}


void CGeneralSettingsDlg::OnCancel()
{
    // TODO: Add specialized code here and/or call the base class
    theApp.m_general_data.update_source = m_update_source_ori;      //Restore starting "Update source" option when clicking "Cancel"

    CTabDlg::OnCancel();
}


void CGeneralSettingsDlg::OnBnClickedRestoreDefaultTimeSpanButton()
{
    // TODO: Add control notification handler code here
    m_monitor_span_edit.SetValue(1000);
}


void CGeneralSettingsDlg::OnCbnSelchangeSelectHardDiskCombo()
{
    // TODO: Add control notification handler code here
    CString hard_disk_name;
    m_hard_disk_combo.GetWindowText(hard_disk_name);
    m_data.hard_disk_name = hard_disk_name.GetString();
}


void CGeneralSettingsDlg::OnBnClickedCpuCheck()
{
    // TODO: Add control notification handler code here
    bool checked = IsDlgButtonChecked(IDC_CPU_CHECK) != 0;
    if (checked && !ShowHardwareMonitorWarning())
    {
        checked = false;
        CheckDlgButton(IDC_CPU_CHECK, FALSE);
    }
    m_data.SetHardwareEnable(HI_CPU, checked);
}


void CGeneralSettingsDlg::OnBnClickedGpuCheck()
{
    // TODO: Add control notification handler code here
    bool checked = IsDlgButtonChecked(IDC_GPU_CHECK) != 0;
    if (checked && !ShowHardwareMonitorWarning())
    {
        checked = false;
        CheckDlgButton(IDC_GPU_CHECK, FALSE);
    }
    m_data.SetHardwareEnable(HI_GPU, checked);
}


void CGeneralSettingsDlg::OnBnClickedHddCheck()
{
    // TODO: Add control notification handler code here
    bool checked = IsDlgButtonChecked(IDC_HDD_CHECK) != 0;
    if (checked && !ShowHardwareMonitorWarning())
    {
        checked = false;
        CheckDlgButton(IDC_HDD_CHECK, FALSE);
    }
    m_data.SetHardwareEnable(HI_HDD, checked);
}


void CGeneralSettingsDlg::OnBnClickedMbdCheck()
{
    // TODO: Add control notification handler code here
    bool checked = IsDlgButtonChecked(IDC_MBD_CHECK) != 0;
    if (checked && !ShowHardwareMonitorWarning())
    {
        checked = false;
        CheckDlgButton(IDC_MBD_CHECK, FALSE);
    }
    m_data.SetHardwareEnable(HI_MBD, checked);

}


void CGeneralSettingsDlg::OnCbnSelchangeSelectCpuCombo()
{
    // TODO: Add control notification handler code here
    CString cpu_core_name;
    m_select_cpu_combo.GetWindowText(cpu_core_name);
    m_data.cpu_core_name = cpu_core_name.GetString();
}


void CGeneralSettingsDlg::OnBnClickedPluginManageButton()
{
    // TODO: Add control notification handler code here
    CPluginManagerDlg dlg;
    dlg.DoModal();
}


void CGeneralSettingsDlg::OnBnClickedShowNotifyIconCheck()
{
    // TODO: Add control notification handler code here
    m_data.show_notify_icon = (IsDlgButtonChecked(IDC_SHOW_NOTIFY_ICON_CHECK) != 0);
}


void CGeneralSettingsDlg::OnBnClickedSelectConnectionsButton()
{
    // TODO: Add control notification handler code here
    CSelectConnectionsDlg dlg(m_data.connections_hide);
    if (dlg.DoModal() == IDOK)
    {
        m_data.connections_hide = dlg.GetData();
    }
}


void CGeneralSettingsDlg::OnBnClickedResetAutoRunButton()
{
    //First delete the automatic run on startup
    theApp.SetAutoRunByRegistry(false);
    theApp.SetAutoRunByTaskScheduler(false);
    if (!theApp.SetAutoRun(true, m_data.auto_run_by_task_scheduler))    //Reset to run automatically on startup
    {
        MessageBox(CCommon::LoadText(IDS_SET_AUTO_RUN_FAILED_WARNING), NULL, MB_ICONWARNING | MB_OK);
        return;
    }
    //Get the path to run automatically at startup
    bool auto_run = theApp.GetAutoRun(&m_auto_run_path, m_data.auto_run_by_task_scheduler);
    //Recheck the "Run automatically at startup" checkbox
    CheckDlgButton(IDC_AUTO_RUN_CHECK, auto_run);
    //Update mouse tips
    AddOrUpdateAutoRunTooltip(false);
}


void CGeneralSettingsDlg::OnEnChangeMonitorSpanEdit()
{
    m_data.monitor_time_span = m_monitor_span_edit.GetValue();
}

void CGeneralSettingsDlg::OnBnClickedAutoRunMethodRegestryRadio()
{
    m_data.auto_run_by_task_scheduler = false;
    m_auto_run_modified = true;
    SetControlEnable();
}

void CGeneralSettingsDlg::OnBnClickedAutoRunMethodTaskScheduleRadio()
{
    m_data.auto_run_by_task_scheduler = true;
    m_auto_run_modified = true;
    SetControlEnable();
}
