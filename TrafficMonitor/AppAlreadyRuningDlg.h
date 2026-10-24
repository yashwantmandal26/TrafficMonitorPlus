#pragma once
#include "BaseDialog.h"

// CAppAlreadyRuningDlg dialog box

class CAppAlreadyRuningDlg : public CBaseDialog
{
    DECLARE_DYNAMIC(CAppAlreadyRuningDlg)

public:
    CAppAlreadyRuningDlg(HWND handel, CWnd* pParent = nullptr);   // Standard constructor
    virtual ~CAppAlreadyRuningDlg();

    // Dialog data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_APP_ALREAD_RUNING_DIALOG };
#endif

private:
    HWND m_handle{};        //Handle to the main window of the running TrafficMonitor process

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual CString GetDialogName() const override;

    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedExitInstButton();
    afx_msg void OnBnClickedOpenSettingsButton();
    afx_msg void OnBnClickedShowHideMainWindowButton();
    afx_msg void OnBnClickedShowHideTaskbarWindowButton();
};
