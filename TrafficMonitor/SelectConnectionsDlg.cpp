// SelectConnectionsDlg.cpp: Implementation file
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "SelectConnectionsDlg.h"
#include "afxdialogex.h"
#include "AdapterCommon.h"

// CSelectConnectionsDlg dialog box

IMPLEMENT_DYNAMIC(CSelectConnectionsDlg, CBaseDialog)

CSelectConnectionsDlg::CSelectConnectionsDlg(const StringSet& connections_hide, CWnd* pParent /*=nullptr*/)
	: m_connections_hide(connections_hide), CBaseDialog(IDD_SELECT_CONNECTIONS_DIALOG, pParent)
{
    std::vector<NetWorkConection> connections;
    CAdapterCommon::GetAdapterInfo(connections);
    for (const auto& item : connections)
        m_all_connection_names.push_back(CCommon::StrToUnicode(item.description.c_str()));
}

CSelectConnectionsDlg::~CSelectConnectionsDlg()
{
}

void CSelectConnectionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST1, m_list_ctrl);
}


CString CSelectConnectionsDlg::GetDialogName() const
{
    //throw std::logic_error("The method or operation is not implemented.");
    return _T("SelectConnectionsDlg");
}

BEGIN_MESSAGE_MAP(CSelectConnectionsDlg, CBaseDialog)
END_MESSAGE_MAP()


// CSelectConnectionsDlg message handler


BOOL CSelectConnectionsDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    // TODO: Add additional initialization here
    SetIcon(theApp.GetMenuIcon(IDI_CONNECTION), FALSE);
    m_list_ctrl.SetItemHeight(0, theApp.DPI(20));

    //Add items to list
    for (const auto& connection_name : m_all_connection_names)
    {
        m_list_ctrl.AddString(connection_name.c_str());
        if (!m_connections_hide.Contains(connection_name))
            m_list_ctrl.SetCheck(m_list_ctrl.GetCount() - 1, TRUE);
        else
            m_list_ctrl.SetCheck(m_list_ctrl.GetCount() - 1, FALSE);
    }


    return TRUE;  // return TRUE unless you set the focus to a control
                  // Exception: OCX property page should return FALSE
}


void CSelectConnectionsDlg::OnOK()
{
    // TODO: Add specialized code here and/or call the base class
    //Save the check status of each item
    bool is_empty{ true };
    for (int i = 0; i < static_cast<int>(m_all_connection_names.size()); i++)
    {
        bool is_checked = (m_list_ctrl.GetCheck(i) != 0);
        if (is_checked)
            is_empty = false;
        m_connections_hide.SetStrContained(m_all_connection_names[i], !is_checked);
    }

    if (is_empty)
    {
        MessageBox(CCommon::LoadText(IDS_SELECT_AT_LEASE_ONE_WARNING), NULL, MB_ICONWARNING | MB_OK);
        return;
    }

    CBaseDialog::OnOK();
}
