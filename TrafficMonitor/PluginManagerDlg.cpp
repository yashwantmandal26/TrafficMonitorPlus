// PluginManagerDlg.cpp: implementation file
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "PluginManagerDlg.h"
#include "FilePathHelper.h"
#include "PluginInfoDlg.h"
#include "WIC.h"


// CPluginManagerDlg dialog box

IMPLEMENT_DYNAMIC(CPluginManagerDlg, CBaseDialog)

CPluginManagerDlg::CPluginManagerDlg(CWnd* pParent /*=nullptr*/)
    : CBaseDialog(IDD_PLUGIN_MANAGER_DIALOG, pParent)
{

}

CPluginManagerDlg::~CPluginManagerDlg()
{
}

void CPluginManagerDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST1, m_list_ctrl);
    DDX_Control(pDX, IDC_PLUGIN_DOWNLOAD_STATIC, m_plugin_download_lnk);
    DDX_Control(pDX, IDC_PLUGIN_DEV_GUID_STATIC, m_plugin_dev_guide_lnk);
    DDX_Control(pDX, IDC_OPEN_PLUGIN_DIR_STATIC, m_open_plugin_dir_lnk);
}

void CPluginManagerDlg::EnableControl()
{
    bool enable{ IsSelectedPluginEnable() };
    EnableDlgCtrl(IDC_OPTINS_BUTTON, enable);
    EnableDlgCtrl(IDC_PLUGIN_INFO_BUTTON, enable);
}

bool CPluginManagerDlg::IsSelectedValid()
{
    return m_item_selected >= 0 && m_item_selected < static_cast<int>(theApp.m_plugins.GetPlugins().size());
}

bool CPluginManagerDlg::IsSelectedPluginEnable()
{
    CPluginManager::PluginInfo plugin_info{};
    bool plugin_loaded{ false };
    if (IsSelectedValid())
    {
        plugin_info = theApp.m_plugins.GetPlugins()[m_item_selected];
        plugin_loaded = (plugin_info.state == CPluginManager::PluginState::PS_SUCCEED);
    }

    return plugin_loaded;
}

CString CPluginManagerDlg::GetDialogName() const
{
    return _T("PluginManagerDlg");
}

bool CPluginManagerDlg::InitializeControls()
{
    RepositionTextBasedControls({
        { CtrlTextInfo::L4, IDC_PLUGIN_INFO_BUTTON, CtrlTextInfo::W32 },
        { CtrlTextInfo::L3, IDC_OPTINS_BUTTON, CtrlTextInfo::W32 }
    });
    RepositionTextBasedControls({
        { CtrlTextInfo::L4, IDC_PLUGIN_DOWNLOAD_STATIC },
        { CtrlTextInfo::L3, IDC_PLUGIN_DEV_GUID_STATIC },
        { CtrlTextInfo::L2, IDC_OPEN_PLUGIN_DIR_STATIC }
    });
    return true;
}


BEGIN_MESSAGE_MAP(CPluginManagerDlg, CBaseDialog)
    ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CPluginManagerDlg::OnNMRClickList1)
    ON_NOTIFY(NM_CLICK, IDC_LIST1, &CPluginManagerDlg::OnNMClickList1)
    ON_BN_CLICKED(IDC_OPTINS_BUTTON, &CPluginManagerDlg::OnBnClickedOptinsButton)
    ON_BN_CLICKED(IDC_PLUGIN_INFO_BUTTON, &CPluginManagerDlg::OnBnClickedPluginInfoButton)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST1, &CPluginManagerDlg::OnNMDblclkList1)
    ON_WM_INITMENU()
    ON_COMMAND(ID_PLUGIN_DETAIL, &CPluginManagerDlg::OnPluginDetail)
    ON_COMMAND(ID_PLUGIN_OPTIONS, &CPluginManagerDlg::OnPluginOptions)
    ON_COMMAND(ID_PLUGIN_DISABLE, &CPluginManagerDlg::OnPluginDisable)
    ON_MESSAGE(WM_LINK_CLICKED, &CPluginManagerDlg::OnLinkClicked)
END_MESSAGE_MAP()


// CPluginManagerDlg message handler


BOOL CPluginManagerDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    // TODO: Add additional initialization here
    SetIcon(theApp.GetMenuIcon(IDI_PLUGINS), FALSE);

    //Initialize list control
    CRect rect;
    m_list_ctrl.GetClientRect(rect);
    m_list_ctrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
    int width0, width1, width2, width3;
    width0 = width1 = rect.Width() * 3 / 10;
    width3 = rect.Width() * 2 / 10;
    width2 = rect.Width() - width0 - width1 - width3 - theApp.DPI(20) - 1;
    m_list_ctrl.InsertColumn(COL_FILE_NAME, CCommon::LoadText(IDS_FILE_NAME), LVCFMT_LEFT, width0);
    m_list_ctrl.InsertColumn(COL_NAME, CCommon::LoadText(IDS_PLUGIN_NAME), LVCFMT_LEFT, width1);
    m_list_ctrl.InsertColumn(COL_VERSION, CCommon::LoadText(IDS_VERSION), LVCFMT_LEFT, width2);
    m_list_ctrl.InsertColumn(COL_STATUS, CCommon::LoadText(IDS_STATUS), LVCFMT_LEFT, width3);

    //add icon
    int item_num = theApp.m_plugins.GetPlugins().size();
    m_plugin_icon_list.Create(theApp.DPI(16), theApp.DPI(16), ILC_COLOR32, item_num, item_num);
    for (const auto& plugin : theApp.m_plugins.GetPlugins())
    {
        HICON hIcon{};
        if (plugin.plugin != nullptr && plugin.plugin->GetAPIVersion() >= 5)
        {
            hIcon = (HICON)plugin.plugin->GetPluginIcon();
        }
        if (hIcon == nullptr)
        {
            if (plugin.state == CPluginManager::PluginState::PS_DISABLE)
                hIcon = theApp.GetMenuIcon(IDI_PLUGIN_DISABLED);
            else
                hIcon = theApp.GetMenuIcon(IDI_PLUGINS);
        }
        m_plugin_icon_list.Add(hIcon);
    }
    m_list_ctrl.SetImageList(&m_plugin_icon_list, LVSIL_SMALL);

    //Insert row into list
    for (const auto& plugin : theApp.m_plugins.GetPlugins())
    {
        std::wstring file_name = CFilePathHelper(plugin.file_path).GetFileName();
        CString error_info;
        error_info.Format(_T("%u %s"), plugin.error_code, CCommon::GetErrorMessage(plugin.error_code).GetString());
        CString status;
        switch (plugin.state)
        {
        case CPluginManager::PluginState::PS_SUCCEED:
            status = CCommon::LoadText(IDS_PLUGIN_LOAD_SUCCEED);
            break;
        case CPluginManager::PluginState::PS_MUDULE_LOAD_FAILED:
            status = CCommon::LoadTextFormat(IDS_PLUGIN_MODULE_LOAD_FAILED, { error_info });
            break;
        case CPluginManager::PluginState::PS_FUNCTION_GET_FAILED:
            status = CCommon::LoadTextFormat(IDS_PLUGIN_FUNCTION_GET_FAILED, { error_info });
            break;
        case CPluginManager::PluginState::PS_DISABLE:
            status = CCommon::LoadText(IDS_DISABLED);
            break;
        case CPluginManager::PluginState::PS_VERSION_NOT_SUPPORT:
            status = CCommon::LoadText(IDS_PLUGIN_VERSION_NOT_SUPPORT);
            break;
        default:
            break;
        }
        int index = m_list_ctrl.GetItemCount();
        m_list_ctrl.InsertItem(index, file_name.c_str(), m_list_ctrl.GetItemCount());
        m_list_ctrl.SetItemText(index, COL_NAME, plugin.Property(ITMPlugin::TMI_NAME).c_str());
        if (plugin.state == CPluginManager::PluginState::PS_SUCCEED)
        {
            //If the plug-in is updated, add the update information after the version number
            PluginVersion cur_version(plugin.Property(ITMPlugin::TMI_VERSION));
            PluginVersion latest_version(theApp.m_plugin_update.GetPluginLatestVersions(file_name));
            if (cur_version < latest_version)
            {
                std::wstring update_info = theApp.m_str_table.LoadTextFormat(IDS_PLUGIN_NEW_VERSION_INFO, { latest_version.GetVersionWString() });
                std::wstring version_text = plugin.Property(ITMPlugin::TMI_VERSION);
                version_text += L" (";
                version_text += update_info;
                version_text += L')';
                m_list_ctrl.SetItemText(index, COL_VERSION, version_text.c_str());
            }
            else
            {
                m_list_ctrl.SetItemText(index, COL_VERSION, plugin.Property(ITMPlugin::TMI_VERSION).c_str());
            }
        }
        m_list_ctrl.SetItemText(index, COL_STATUS, status);
    }

    if (theApp.m_general_data.update_source == 1)   //The update source is Gitee, jump to the link to Gitee
        m_plugin_download_lnk.SetURL(L"https://gitee.com/zhongyang219/TrafficMonitorPlugins/blob/main/download/plugin_download.md");
    else //Update source is Github
        m_plugin_download_lnk.SetURL(L"https://github.com/zhongyang219/TrafficMonitorPlugins/blob/main/download/plugin_download.md");
    m_plugin_dev_guide_lnk.SetURL(L"https://github.com/zhongyang219/TrafficMonitor/wiki/%E6%8F%92%E4%BB%B6%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97");
    m_open_plugin_dir_lnk.SetLinkIsURL(false);

    EnableControl();

    CCommon::LoadMenuResource(m_menu, IDR_PLUGIN_MANAGER_MENU); //Load right-click menu

    //settings menu icon
    CMenuIcon::AddIconToMenuItem(m_menu.GetSubMenu(0)->GetSafeHmenu(), ID_PLUGIN_DETAIL, FALSE, theApp.GetMenuIcon(IDI_INFO));
    CMenuIcon::AddIconToMenuItem(m_menu.GetSubMenu(0)->GetSafeHmenu(), ID_PLUGIN_OPTIONS, FALSE, theApp.GetMenuIcon(IDI_SETTINGS));

    //Set button icon
    SetButtonIcon(IDC_PLUGIN_INFO_BUTTON, theApp.GetMenuIcon(IDI_INFO));
    SetButtonIcon(IDC_OPTINS_BUTTON, theApp.GetMenuIcon(IDI_SETTINGS));

    return TRUE;  // return TRUE unless you set the focus to a control
                  // Exception: OCX property page should return FALSE
}


void CPluginManagerDlg::OnNMRClickList1(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: Add control notification handler code here
    m_item_selected = pNMItemActivate->iItem;
    EnableControl();

    //Pop up right-click menu
    CMenu* pContextMenu = m_menu.GetSubMenu(0);	//Get the first popup menu
    pContextMenu->SetDefaultItem(ID_PLUGIN_DETAIL);
    //Update plugin submenu
    if (m_item_selected >= 0 && m_item_selected < static_cast<int>(theApp.m_plugins.GetPlugins().size()))
    {
        auto plugin_info = theApp.m_plugins.GetPlugins()[m_item_selected];
        theApp.UpdatePluginMenu(pContextMenu, plugin_info.plugin, 3);
    }

    CPoint point1;	//Define a position used to determine the cursor position
    GetCursorPos(&point1);	//Get the current cursor position so that the menu can follow the cursor
    pContextMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point1.x, point1.y, this); //Show popup menu at specified location

    *pResult = 0;
}


void CPluginManagerDlg::OnNMClickList1(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: Add control notification handler code here
    m_item_selected = pNMItemActivate->iItem;
    EnableControl();
    *pResult = 0;
}


void CPluginManagerDlg::OnBnClickedOptinsButton()
{
    // TODO: Add control notification handler code here
    if (m_item_selected >= 0 && m_item_selected < static_cast<int>(theApp.m_plugins.GetPlugins().size()))
    {
        auto plugin_info = theApp.m_plugins.GetPlugins()[m_item_selected];
        if (plugin_info.plugin != nullptr)
        {
            ITMPlugin::OptionReturn rtn = plugin_info.plugin->ShowOptionsDialog(m_hWnd);
            if (rtn == ITMPlugin::OR_OPTION_NOT_PROVIDED)
                MessageBox(CCommon::LoadText(IDS_PLUGIN_NO_OPTIONS_INFO), nullptr, MB_ICONINFORMATION | MB_OK);
            //else if (rtn == ITMPlugin::OR_OPTION_CHANGED)
            //    theApp.m_pMainWnd->SendMessage(WM_REOPEN_TASKBAR_WND);
        }
    }
}


void CPluginManagerDlg::OnBnClickedPluginInfoButton()
{
    // TODO: Add control notification handler code here
    if (IsSelectedPluginEnable())
    {
        CPluginInfoDlg dlg(m_item_selected);
        dlg.DoModal();
    }
}


void CPluginManagerDlg::OnNMDblclkList1(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: Add control notification handler code here
    m_item_selected = pNMItemActivate->iItem;
    OnBnClickedPluginInfoButton();
    *pResult = 0;
}


void CPluginManagerDlg::OnInitMenu(CMenu* pMenu)
{
    CBaseDialog::OnInitMenu(pMenu);

    // TODO: add message handler code here
    bool enable{ IsSelectedPluginEnable() };
    pMenu->EnableMenuItem(ID_PLUGIN_DETAIL, MF_BYCOMMAND | (enable ? MF_ENABLED : MF_GRAYED));
    pMenu->EnableMenuItem(ID_PLUGIN_OPTIONS, MF_BYCOMMAND | (enable ? MF_ENABLED : MF_GRAYED));
    pMenu->EnableMenuItem(ID_PLUGIN_DISABLE, MF_BYCOMMAND | (IsSelectedValid() ? MF_ENABLED : MF_GRAYED));

    bool disabled{};
    CPluginManager::PluginInfo plugin_info;
    if (m_item_selected >= 0 && m_item_selected < static_cast<int>(theApp.m_plugins.GetPlugins().size()))
    {
        plugin_info = theApp.m_plugins.GetPlugins()[m_item_selected];
        std::wstring file_name = CFilePathHelper(plugin_info.file_path).GetFileName();
        disabled = theApp.m_cfg_data.plugin_disabled.Contains(file_name);
    }
    pMenu->CheckMenuItem(ID_PLUGIN_DISABLE, MF_BYCOMMAND | (disabled ? MF_CHECKED : MF_UNCHECKED));
}


void CPluginManagerDlg::OnPluginDetail()
{
    OnBnClickedPluginInfoButton();
}


void CPluginManagerDlg::OnPluginOptions()
{
    OnBnClickedOptinsButton();
}


void CPluginManagerDlg::OnPluginDisable()
{
    if (m_item_selected >= 0 && m_item_selected < static_cast<int>(theApp.m_plugins.GetPlugins().size()))
    {
        CPluginManager::PluginInfo plugin_info = theApp.m_plugins.GetPlugins()[m_item_selected];
        std::wstring file_name = CFilePathHelper(plugin_info.file_path).GetFileName();
        bool disabled = theApp.m_cfg_data.plugin_disabled.Contains(file_name);
        theApp.m_cfg_data.plugin_disabled.SetStrContained(file_name, !disabled);
        MessageBox(CCommon::LoadText(IDS_RESTART_TO_APPLY_CHANGE_INFO), nullptr, MB_OK | MB_ICONINFORMATION);
    }
}


afx_msg LRESULT CPluginManagerDlg::OnLinkClicked(WPARAM wParam, LPARAM lParam)
{
    CWnd* pCtrl = (CWnd*)wParam;
    //Clicked "Open plugin directory"
    if (pCtrl == &m_open_plugin_dir_lnk)
    {
        wstring plugin_dir = CCommon::GetModuleDir() + L"plugins";
        CreateDirectory(plugin_dir.c_str(), NULL);       //If plugins does not exist, create it
        ShellExecute(NULL, _T("open"), _T("explorer"), plugin_dir.c_str(), NULL, SW_SHOWNORMAL);
    }

    return 0;
}


BOOL CPluginManagerDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    UINT uMsg = LOWORD(wParam);
    //Plugin command selected
    if (uMsg >= ID_PLUGIN_COMMAND_START && uMsg <= ID_PLUGIN_COMMAND_MAX)
    {
        int index = uMsg - ID_PLUGIN_COMMAND_START;
        if (m_item_selected >= 0 && m_item_selected < static_cast<int>(theApp.m_plugins.GetPlugins().size()))
        {
            ITMPlugin* plugin = theApp.m_plugins.GetPlugins()[m_item_selected].plugin;
            if (plugin != nullptr && plugin->GetAPIVersion() >= 5)
            {
                plugin->OnPluginCommand(index, (void*)GetSafeHwnd(), nullptr);
            }
        }
    }

    return CBaseDialog::OnCommand(wParam, lParam);
}
