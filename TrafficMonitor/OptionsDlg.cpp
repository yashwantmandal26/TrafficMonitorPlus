// OptionsDlg.cpp: Implementation file
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "OptionsDlg.h"
#include "afxdialogex.h"


// COptionsDlg dialog box

IMPLEMENT_DYNAMIC(COptionsDlg, CBaseDialog)

COptionsDlg::COptionsDlg(int tab, CWnd* pParent /*=NULL*/)
    : CBaseDialog(IDD_OPTIONS_DIALOG, pParent), m_tab_selected{ tab }
{

}

COptionsDlg::~COptionsDlg()
{
}

CString COptionsDlg::GetDialogName() const
{
    return OPTION_DLG_NAME;
}

void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TAB1, m_tab);
}


BEGIN_MESSAGE_MAP(COptionsDlg, CBaseDialog)
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_APPLY_BUTTON, &COptionsDlg::OnBnClickedApplyButton)
END_MESSAGE_MAP()


// COptionsDlg message handler


BOOL COptionsDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    // TODO: Add additional initialization here
    SetIcon(theApp.GetMenuIcon(IDI_SETTINGS), FALSE);       // Set small icon

    //Create subdialog
    m_tab1_dlg.Create(IDD_MAIN_WND_SETTINGS_DIALOG, &m_tab);
    m_tab2_dlg.Create(IDD_TASKBAR_SETTINGS_DIALOG, &m_tab);
    m_tab3_dlg.Create(IDD_GENERAL_SETTINGS_DIALOG, &m_tab);

    //Save subdialog
    m_tab_vect.push_back(&m_tab1_dlg);
    m_tab_vect.push_back(&m_tab2_dlg);
    m_tab_vect.push_back(&m_tab3_dlg);

    //Get the initial height of the subdialog
    for (const auto* pDlg : m_tab_vect)
    {
        CRect rect;
        pDlg->GetWindowRect(rect);
        m_tab_height.push_back(rect.Height());
    }

    //Add dialog
    m_tab.AddWindow(&m_tab1_dlg, CCommon::LoadText(IDS_MAIN_WINDOW_SETTINGS));
    m_tab.AddWindow(&m_tab2_dlg, CCommon::LoadText(IDS_TASKBAR_WINDOW_SETTINGS));
    m_tab.AddWindow(&m_tab3_dlg, CCommon::LoadText(IDS_GENERAL_SETTINGS));

    //Add an icon to each label
    CImageList ImageList;
    ImageList.Create(theApp.DPI(16), theApp.DPI(16), ILC_COLOR32 | ILC_MASK, 2, 2);
    ImageList.Add(theApp.GetMenuIcon(IDI_MAIN_WINDOW));
    ImageList.Add(theApp.GetMenuIcon(IDI_TASKBAR_WINDOW));
    ImageList.Add(theApp.GetMenuIcon(IDI_SETTINGS));
    m_tab.SetImageList(&ImageList);
    ImageList.Detach();

    m_tab.SetItemSize(CSize(theApp.DPI(60), theApp.DPI(24)));
    m_tab.AdjustTabWindowSize();

    //Set scrolling information for each child window
    for (size_t i = 0; i < m_tab_vect.size(); i++)
    {
        m_tab_vect[i]->SetScrollbarInfo(m_tab.m_tab_rect.Height(), m_tab_height[i]);
    }

    //Set the default selected tags
    if (m_tab_selected < 0 || m_tab_selected >= m_tab.GetItemCount())
        m_tab_selected = 0;
    m_tab.SetCurTab(m_tab_selected);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // Exception: OCX property page should return FALSE
}


void COptionsDlg::OnOK()
{
    // TODO: Add specialized code here and/or call the base class
    m_tab1_dlg.OnOK();
    m_tab2_dlg.OnOK();
    m_tab3_dlg.OnOK();

    CBaseDialog::OnOK();
}


void COptionsDlg::OnSize(UINT nType, int cx, int cy)
{
    CBaseDialog::OnSize(nType, cx, cy);

    // TODO: add message handler code here
    if (nType != SIZE_MINIMIZED)
    {
        //Set scrolling information for each child window
        for (size_t i = 0; i < m_tab_vect.size(); i++)
        {
            m_tab_vect[i]->SetScrollbarInfo(m_tab.m_tab_rect.Height(), m_tab_height[i]);
        }
    }
}


void COptionsDlg::OnCancel()
{
    // TODO: Add specialized code here and/or call the base class
    m_tab3_dlg.OnCancel();

    CBaseDialog::OnCancel();
}


void COptionsDlg::OnBnClickedApplyButton()
{
    m_tab2_dlg.SaveColorSettingToDefaultStyle();
    ::SendMessage(theApp.m_pMainWnd->GetSafeHwnd(), WM_SETTINGS_APPLIED, (WPARAM)this, 0);
    for (size_t i = 0; i < m_tab_vect.size(); i++)
    {
        m_tab_vect[i]->OnSettingsApplied();
    }
}
