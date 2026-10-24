#pragma once
#include "afxdialogex.h"
#include "SpinEdit.h"
#include "BaseDialog.h"

// CWin11TaskbarSettingDlg dialog box

class CWin11TaskbarSettingDlg : public CBaseDialog
{
	DECLARE_DYNAMIC(CWin11TaskbarSettingDlg)

public:
	CWin11TaskbarSettingDlg(TaskBarSettingData& data, CWnd* pParent = nullptr);   // Standard constructor
	virtual ~CWin11TaskbarSettingDlg();

// Dialog data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WIN11_TASKBAR_SETTING_DLG };
#endif

private:
    TaskBarSettingData& m_data;
    CSpinEdit m_window_offset_top_edit;
    CSpinEdit m_window_offset_left_edit;
    CSpinEdit m_widgets_width_edit;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual CString GetDialogName() const override;
    virtual bool InitializeControls() override;

    void EnableDlgCtrl(UINT id, bool enable);

	DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnBnClickedRestoreDefaultButton();
};
