// HistoryTrafficDlg.cpp: implementation file
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "HistoryTrafficDlg.h"
#include "afxdialogex.h"


// CHistoryTrafficDlg dialog box

IMPLEMENT_DYNAMIC(CHistoryTrafficDlg, CBaseDialog)

CHistoryTrafficDlg::CHistoryTrafficDlg(deque<HistoryTraffic>& history_traffics, CWnd* pParent /*=NULL*/)
    : CBaseDialog(IDD_HISTORY_TRAFFIC_DIALOG, pParent), m_history_traffics(history_traffics), m_tab1_dlg(history_traffics, this), m_tab2_dlg(history_traffics, this)
{

}

CHistoryTrafficDlg::~CHistoryTrafficDlg()
{
}


CString CHistoryTrafficDlg::GetDialogName() const
{
    return _T("HistoryTrafficDlg");
}

void CHistoryTrafficDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TAB1, m_tab);
}

void CHistoryTrafficDlg::SetTabWndSize()
{
    CRect rect;
    m_tab.GetClientRect(rect);
    CRect rcTabItem;
    m_tab.GetItemRect(0, rcTabItem);
    rect.top += rcTabItem.Height() + 4;
    rect.left += 4;
    rect.bottom -= 4;
    rect.right -= 4;
    m_tab1_dlg.MoveWindow(&rect);
    m_tab2_dlg.MoveWindow(&rect);
}


BEGIN_MESSAGE_MAP(CHistoryTrafficDlg, CBaseDialog)
    ON_WM_GETMINMAXINFO()
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CHistoryTrafficDlg::OnTcnSelchangeTab1)
    ON_WM_SIZE()
END_MESSAGE_MAP()


// CHistoryTrafficDlg message handler


BOOL CHistoryTrafficDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    // TODO: Add additional initialization here
    SetIcon(theApp.GetMenuIcon(IDI_STATISTICS), FALSE);		// Set small icon

    //Insert tag
    m_tab.InsertItem(0, CCommon::LoadText(IDS_LIST_VIEW));
    m_tab.InsertItem(1, CCommon::LoadText(IDS_CALENDAR_VIEW));
    //Create subdialog
    m_tab1_dlg.Create(IDD_HISTORY_TRAFFIC_LIST_DIALOG, &m_tab);
    m_tab2_dlg.Create(IDD_HISTORY_TRAFFIC_CALENDAR_DIALOG, &m_tab);
    //Adjust the size and position of subdialog boxes
    SetTabWndSize();
    //Set the default selected tags
    switch (m_tab_selected)
    {
    case 0:
        m_tab1_dlg.ShowWindow(SW_SHOW);
        break;
    case 1:
        m_tab2_dlg.ShowWindow(SW_SHOW);
        break;
    }
    m_tab.SetCurFocus(m_tab_selected);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // Exception: OCX property page should return FALSE
}



BOOL CHistoryTrafficDlg::PreTranslateMessage(MSG* pMsg)
{
    // TODO: Add specialized code here and/or call the base class
    //if (GetKeyState(VK_CONTROL) & 0x80)
    //{
    //	if (pMsg->wParam == 'D')
    //	{
    //		HistoryTraffic h{};
    //		h.year = 2018;
    //		h.month = 4;
    //		h.day = 29;
    //		auto iter = std::lower_bound(m_history_traffics.begin(), m_history_traffics.end(), h, HistoryTraffic::DateGreater);
    //		int index = iter - m_history_traffics.begin();
    //	}
    //}

    return CBaseDialog::PreTranslateMessage(pMsg);
}

void CHistoryTrafficDlg::OnTcnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult)
{
    // TODO: Add control notification handler code here
    m_tab_selected = m_tab.GetCurSel();
    switch (m_tab_selected)
    {
    case 0:
        m_tab1_dlg.ShowWindow(SW_SHOW);
        m_tab2_dlg.ShowWindow(SW_HIDE);
        m_tab1_dlg.SetFocus();
        break;
    case 1:
        m_tab2_dlg.ShowWindow(SW_SHOW);
        m_tab1_dlg.ShowWindow(SW_HIDE);
        m_tab2_dlg.SetFocus();
        break;
    }
    *pResult = 0;
}


void CHistoryTrafficDlg::OnSize(UINT nType, int cx, int cy)
{
    CBaseDialog::OnSize(nType, cx, cy);

    // TODO: add message handler code here
    if (nType != SIZE_MINIMIZED && m_tab1_dlg.GetSafeHwnd() != NULL && m_tab2_dlg.GetSafeHwnd() != NULL)
    {
        SetTabWndSize();
    }
}
