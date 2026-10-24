//This is the subdialog class used in the Tab tag
#pragma once
#include "BaseDialog.h"

class CTabDlg : public CBaseDialog
{
    DECLARE_DYNAMIC(CTabDlg)
public:
    CTabDlg(UINT nIDTemplate, CWnd *pParent = NULL);
    ~CTabDlg();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL OnInitDialog();
    CWnd* GetParentWindow();
    void SetScrollbarInfo(int nPage, int nMax);
    virtual void OnTabEntered() {}      //Called when the tab switches to the current window
    virtual void OnOK();
    void SetScrollEnable(bool enable) { m_scroll_enable = enable; }
    virtual void SetControlMouseWheelEnable(bool enable) {}     //Override this function in a derived class to set whether the control is allowed to respond to the mouse wheel
    virtual void OnSettingsApplied() {}   //Click the "Apply" button and be called after applying the settings.

    bool IsScrollBarVisible();    //Is the scroll bar visible?

private:
    void ScrollWindowSimple(int step, bool absolute = false);

protected:
    void EnableDlgCtrl(UINT id, bool enable);
    void ShowDlgCtrl(UINT id, bool show);

protected:
    CWnd* m_pParent;
    bool m_scroll_enable{ false };
    int m_last_pos{};

protected:
    // The virtual method of CWnd is declared in afxwin.h, implemented in wincore.cpp, and is called by CWnd during WM_SIZE message processing.
    // Internally, after some judgments, the CMFCDynamicLayout pointer held by CWnd is used to execute m_pDynamicLayout->Adjust()
    virtual void ResizeDynamicLayout() override final;  // The purpose of rewriting here is to change the calling timing from before the default window OnSize to after the parent window OnSize.

public:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnSize(UINT nType, int cx, int cy);
};

