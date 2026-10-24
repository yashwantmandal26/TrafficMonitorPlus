#pragma once
#include "TaskBarDlg.h"
class CWin11TaskbarDlg :
    public CTaskBarDlg
{
public:

private:
    // Inherited via CTaskBarDlg
    void InitTaskbarWnd() override;
    virtual void AdjustTaskbarWndPos(bool force_adjust) override;
    void ResetTaskbarPos() override;
    virtual HWND GetParentHwnd() override;

    HWND m_hNotify;     //Handle to the taskbar notification area
    HWND m_hStart;      //Start button handle
    CRect m_rcNotify;   //The rectangular area of the taskbar notification area
    CRect m_rcStart;     //The rectangular area of the start button
    int m_last_notify_width{};
    int m_last_start_pos{};

    // Inherited via CTaskBarDlg
    void CheckTaskbarOnTopOrBottom() override;

};

