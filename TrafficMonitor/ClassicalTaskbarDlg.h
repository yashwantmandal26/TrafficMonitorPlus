#pragma once
#include "TaskBarDlg.h"
class CClassicalTaskbarDlg :
    public CTaskBarDlg
{
public:

private:
    // Inherited via CTaskBarDlg
    virtual void AdjustTaskbarWndPos(bool force_adjust) override;
    void InitTaskbarWnd() override;
    void ResetTaskbarPos() override;
    virtual HWND GetParentHwnd() override;

private:
    CRect m_rcMinOri;   //Minimizes the rectangular area of the window in the initial state
    int m_left_space{};			//Minimize window and secondary window left margin
    int m_top_space{};			//Margins at the top of the window for minimized windows and secondary windows (used when the taskbar is on the left or right side of the screen)
    HWND m_hBar;		//The handle of the secondary container of the taskbar window
    HWND m_hMin;		//Minimize window handle
    CRect m_rcBar;		//The rectangular area of the taskbar window in the initial state
    CRect m_rcMin;		//Minimize the rectangular area of the window

    int m_last_width;	//Used to detect the last width of the width change
    int m_last_height;	//The last height used to detect height changes (used when the taskbar is on the left or right side of the screen)

    // Inherited via CTaskBarDlg
    void CheckTaskbarOnTopOrBottom() override;
};

