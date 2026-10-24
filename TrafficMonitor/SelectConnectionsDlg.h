#pragma once
#include "CommonData.h"
#include "BaseDialog.h"

// CSelectConnectionsDlg dialog box

class CSelectConnectionsDlg : public CBaseDialog
{
	DECLARE_DYNAMIC(CSelectConnectionsDlg)

public:
	CSelectConnectionsDlg(const StringSet& connections_hide, CWnd* pParent = nullptr);   // Standard constructor
	virtual ~CSelectConnectionsDlg();

    const StringSet& GetData() const { return m_connections_hide; }

// Dialog data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SELECT_CONNECTIONS_DIALOG };
#endif

private:
    CCheckListBox m_list_ctrl;
    std::vector<std::wstring> m_all_connection_names;
    StringSet m_connections_hide;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual CString GetDialogName() const override;

	DECLARE_MESSAGE_MAP()

public:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
};
