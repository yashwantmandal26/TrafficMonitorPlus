#pragma once
#include "ColorStatic.h"
#include "afxwin.h"
#include "ColorSettingListCtrl.h"
#include "BaseDialog.h"

// CTaskbarColorDlg dialog box

class CTaskbarColorDlg : public CBaseDialog
{
	DECLARE_DYNAMIC(CTaskbarColorDlg)

public:
	CTaskbarColorDlg(const std::map<CommonDisplayItem, TaskbarItemColor>& colors, CWnd* pParent = NULL);   // Standard constructor
	virtual ~CTaskbarColorDlg();

    const std::map<CommonDisplayItem, TaskbarItemColor>& GetColors() const { return m_colors; }

	// Dialog data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TASKBAR_COLOR_DIALOG };
#endif

protected:
    std::map<CommonDisplayItem, TaskbarItemColor> m_colors;
    CColorSettingListCtrl m_list_ctrl;

    virtual CString GetDialogName() const override;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
    afx_msg void OnNMDblclkList1(NMHDR *pNMHDR, LRESULT *pResult);
};
