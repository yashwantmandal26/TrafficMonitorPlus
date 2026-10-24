// OptionsDlg.cpp: Implementation file
//

#include "pch.h"
#include "PluginDemo.h"
#include "OptionsDlg.h"
#include "afxdialogex.h"


// COptionsDlg dialog box

IMPLEMENT_DYNAMIC(COptionsDlg, CDialog)

COptionsDlg::COptionsDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_OPTIONS_DIALOG, pParent)
{

}

COptionsDlg::~COptionsDlg()
{
}

void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
    ON_BN_CLICKED(IDC_SHOW_SECOND_CHECK, &COptionsDlg::OnBnClickedShowSecondCheck)
END_MESSAGE_MAP()


// COptionsDlg message handler


BOOL COptionsDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // TODO: Add additional initialization here
    //Initialize control state
    CheckDlgButton(IDC_SHOW_SECOND_CHECK, m_data.show_second);
    //CheckDlgButton(IDC_SHOW_LABEL_CHECK, m_data.show_label_text);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // Exception: OCX property page should return FALSE
}


void COptionsDlg::OnBnClickedShowSecondCheck()
{
    // TODO: Add control notification handler code here
    m_data.show_second = (IsDlgButtonChecked(IDC_SHOW_SECOND_CHECK) != 0);
}
