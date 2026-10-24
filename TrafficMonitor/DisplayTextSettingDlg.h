#pragma once
#include "BaseDialog.h"
#include "CommonData.h"
#include "ListCtrlEx.h"

// CDisplayTextSettingDlg dialog box

class CDisplayTextSettingDlg : public CBaseDialog
{
	DECLARE_DYNAMIC(CDisplayTextSettingDlg)

public:
	CDisplayTextSettingDlg(DispStrings& display_texts, bool main_window_text = false, CWnd* pParent = nullptr);   // Standard constructor
	virtual ~CDisplayTextSettingDlg();

// Dialog data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DISPLAY_TEXT_SETTINGS_DIALOG };
#endif

private:
    DispStrings& m_display_texts;

    CListCtrlEx m_list_ctrl;
    bool m_main_window_text{ false };       //If true, it is set for the main window text, otherwise it is set for the taskbar window.
    CMenu m_menu;
    int m_item_selected{ -1 };

protected:
    virtual CString GetDialogName() const override;
    CommonDisplayItem GetDisplayItem(int row);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnBnClickedRestoreDefaultButton();
    afx_msg void OnRestoreDefault();
    afx_msg void OnNMRClickList1(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnInitMenu(CMenu* pMenu);
    afx_msg void OnNMClickList1(NMHDR* pNMHDR, LRESULT* pResult);
};
