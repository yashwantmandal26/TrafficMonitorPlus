#pragma once
#include "TabDlg.h"
#include "SpinEdit.h"
#include "ComboBox2.h"

// CGeneralSettingsDlg dialog

class CGeneralSettingsDlg : public CTabDlg
{
    DECLARE_DYNAMIC(CGeneralSettingsDlg)

public:
    CGeneralSettingsDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CGeneralSettingsDlg();

    static void CheckTaskbarDisplayItem();

    //option setting data
    GeneralSettingData m_data;

    // Dialog data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_GENERAL_SETTINGS_DIALOG };
#endif

public:
    bool IsAutoRunModified() const { return m_auto_run_modified; }
    bool IsShowAllInterfaceModified() const { return m_show_all_interface_modified; }
    bool IsMonitorTimeSpanModified() const;
    //bool IsTaskbarItemModified() const { return m_taskbar_item_modified; }

protected:
    bool m_auto_run_modified{ false };      //If the settings for automatic operation at startup are changed, it will be set to true
    bool m_show_all_interface_modified{ false };
    int m_monitor_time_span_ori{};
    int m_update_source_ori{};
    //bool m_taskbar_item_modified{ false };
    wstring m_auto_run_path;

    //control variable
    CSpinEdit m_traffic_tip_edit;
    CComboBox2 m_traffic_tip_combo;
    CSpinEdit m_memory_tip_edit;
    CComboBox2 m_language_combo;
    CToolTipCtrl m_toolTip;
    CSpinEdit m_monitor_span_edit;
    CSpinEdit m_cpu_temp_tip_edit;
    CSpinEdit m_gpu_temp_tip_edit;
    CSpinEdit m_hdd_temp_tip_edit;
    CSpinEdit m_mbd_temp_tip_edit;
    CComboBox2 m_hard_disk_combo;
    CComboBox2 m_select_cpu_combo;
    CButton m_plugin_manager_btn;
    CButton m_select_connection_btn;

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    //Set the enabling and disabling of controls
    void SetControlEnable();

    virtual void SetControlMouseWheelEnable(bool enable) override;
    virtual void OnSettingsApplied() override;
    virtual bool InitializeControls() override;

    //Displays the prompt when hardware monitoring is turned on. If the user selects "Yes", it returns true, otherwise it returns false.
    //The "Don't show this dialog again" flag is saved in the registry "\HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\DontShowMeThisDialogAgain"
    bool ShowHardwareMonitorWarning();

    //Add or update mouse prompts that run automatically at startup
    void AddOrUpdateAutoRunTooltip(bool add);

    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedCheckNowButton();
    afx_msg void OnBnClickedCheckUpdateCheck();
    afx_msg void OnBnClickedAutoRunCheck();
    virtual void OnOK();
    afx_msg void OnBnClickedTodayTrafficTipCheck();
    afx_msg void OnBnClickedMemoryUsageTipCheck();
    afx_msg void OnBnClickedOpenConfigPathButton();
    afx_msg void OnBnClickedShowAllConnectionCheck();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnBnClickedUseCpuTimeRadio();
    afx_msg void OnBnClickedUsePdhRadio();
    afx_msg LRESULT OnSpinEditPosChanged(WPARAM wParam, LPARAM lParam);
    afx_msg void OnEnKillfocusMonitorSpanEdit();
    afx_msg void OnBnClickedCpuTempTipCheck();
    afx_msg void OnBnClickedGpuTempTipCheck();
    afx_msg void OnBnClickedHddTempTipCheck();
    afx_msg void OnBnClickedMbdTempTipCheck();
    afx_msg void OnBnClickedGithubRadio();
    afx_msg void OnBnClickedGiteeRadio();
    virtual void OnCancel();
    afx_msg void OnBnClickedRestoreDefaultTimeSpanButton();
    afx_msg void OnCbnSelchangeSelectHardDiskCombo();
    afx_msg void OnBnClickedCpuCheck();
    afx_msg void OnBnClickedGpuCheck();
    afx_msg void OnBnClickedHddCheck();
    afx_msg void OnBnClickedMbdCheck();
    afx_msg void OnCbnSelchangeSelectCpuCombo();
    afx_msg void OnBnClickedPluginManageButton();
    afx_msg void OnBnClickedShowNotifyIconCheck();
    afx_msg void OnBnClickedSelectConnectionsButton();
    afx_msg void OnBnClickedResetAutoRunButton();
    afx_msg void OnEnChangeMonitorSpanEdit();
protected:
public:
    afx_msg void OnBnClickedAutoRunMethodRegestryRadio();
    afx_msg void OnBnClickedAutoRunMethodTaskScheduleRadio();
};
