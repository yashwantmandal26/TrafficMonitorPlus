#include "stdafx.h"
#include "TabDlg.h"
#include "TrafficMonitor.h"

IMPLEMENT_DYNAMIC(CTabDlg, CBaseDialog)

CTabDlg::CTabDlg(UINT nIDTemplate, CWnd * pParent) : CBaseDialog(nIDTemplate, pParent)
{
    m_pParent = pParent;
}


CTabDlg::~CTabDlg()
{
}


BOOL CTabDlg::PreTranslateMessage(MSG* pMsg)
{
    // TODO: Add specialized code here and/or call the base class
    //Since this is a sub-dialog box in the tab tag, the user should not respond to the IDOK or IDCANCEL of the current dialog box after pressing Enter or ESC.
    //Instead, it should respond to the IDOK or IDCANCEL command of the tab tag's parent window.
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
    {
        if (m_pParent != nullptr)
            m_pParent->SendMessage(WM_COMMAND, IDCANCEL);
        return TRUE;
    }
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
    {
        if (m_pParent != nullptr)
            m_pParent->SendMessage(WM_COMMAND, IDOK);
        return TRUE;
    }

    return CBaseDialog::PreTranslateMessage(pMsg);
}


BOOL CTabDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    // TODO: Add additional initialization here
    //Set window background to white
    if (theApp.m_win_version.IsWindows11OrLater())
        SetBackgroundColor(RGB(249, 249, 249));
    else
        SetBackgroundColor(RGB(255, 255, 255));

    return TRUE;  // return TRUE unless you set the focus to a control
                  // Exception: OCX property page should return FALSE
}

CWnd* CTabDlg::GetParentWindow()
{
    CWnd* pParent = GetParent();
    if (pParent != nullptr)
    {
        return pParent->GetParent();
    }
    return nullptr;
}

void CTabDlg::SetScrollbarInfo(int nPage, int nMax)
{
    // Get scroll bar status
    SCROLLINFO scrollinfo;
    GetScrollInfo(SB_VERT, &scrollinfo, SIF_ALL);
    if (m_scroll_enable)
        m_last_pos = scrollinfo.nPos - scrollinfo.nMin;     // If scrolling is enabled, record the current scroll position
    else
        m_scroll_enable = true;                             // Otherwise enable scrolling
    int reset_pos = m_last_pos;
    // New scroll state bounds check after parent window OnSize
    if (nMax < 0) nMax = 0;
    if (m_last_pos > nMax - nPage) m_last_pos = nMax - nPage;
    if (m_last_pos < 0) m_last_pos = 0;
    // Set scroll bar state
    scrollinfo.nMin = 0;                // Set the starting point of scrolling content at pos in the window
    scrollinfo.nMax = nMax;             // Set the end point of the scrolling content to pos in the window
    scrollinfo.nPage = nPage;           // Set the actual display content area height
    scrollinfo.nPos = m_last_pos;       // Set the top pos of the currently displayed content area
    SetScrollInfo(SB_VERT, &scrollinfo, SIF_ALL);
    // Reset scrolling
    if (reset_pos != 0)
        ScrollWindow(0, reset_pos);
    // Instead of the framework responding to OnSize (after SetScrollInfo determines and sets whether the scrollbar is displayed in the new state)
    CBaseDialog::ResizeDynamicLayout(); // This method will ignore the scrolling and update the position of the control, causing the control to be misaligned. Call it when the scroll is 0 to avoid this problem.
    // perform scrolling
    if (m_last_pos != 0)
        ScrollWindow(0, -m_last_pos);
}

void CTabDlg::ScrollWindowSimple(int step, bool absolute)
{
    SCROLLINFO scrollinfo;
    GetScrollInfo(SB_VERT, &scrollinfo, SIF_ALL);
    if (step == INT_MAX)
        step = scrollinfo.nPage;
    if (step == INT_MIN)
        step = -static_cast<int>(scrollinfo.nPage);
    int offset_diff = scrollinfo.nMax - static_cast<int>(scrollinfo.nPage);
    if (offset_diff < 0)    //nMax should always be greater than or equal to nPage, there is no scroll bar when nMax is equal to nPage
        offset_diff = 0;
    if (!absolute)
        scrollinfo.nPos -= step;
    else
    {
        if (step < 0)               // In absolute position, step is -1, which means scrolling to the bottom.
            step = offset_diff;
        scrollinfo.nPos = step;     // At this time, stpe is the target absolute position.
        step = m_last_pos - step;   // Convert the absolute position of the parameter to an offset to continue processing
    }
    if (scrollinfo.nPos < scrollinfo.nMin)                                          // Limit nPos to not be less than the minimum value nMin
    {
        step -= scrollinfo.nMin - scrollinfo.nPos;
        scrollinfo.nPos = scrollinfo.nMin;
    }
    if (scrollinfo.nPos > offset_diff)     // Limit nPos to not be greater than the maximum value nMax - nPage
    {
        step += scrollinfo.nPos - offset_diff;
        scrollinfo.nPos = offset_diff;
    }
    SetScrollInfo(SB_VERT, &scrollinfo, SIF_ALL);
    m_last_pos = scrollinfo.nPos;
    ScrollWindow(0, step);
}

void CTabDlg::EnableDlgCtrl(UINT id, bool enable)
{
    CWnd* pCtrl = GetDlgItem(id);
    if (pCtrl != nullptr)
        pCtrl->EnableWindow(enable);
}

void CTabDlg::ShowDlgCtrl(UINT id, bool show)
{
    CWnd* pCtrl = GetDlgItem(id);
    if (pCtrl != nullptr)
        pCtrl->ShowWindow(show);
}

BEGIN_MESSAGE_MAP(CTabDlg, CBaseDialog)
    ON_WM_VSCROLL()
    ON_WM_MOUSEWHEEL()
    ON_WM_SIZE()
END_MESSAGE_MAP()


void CTabDlg::ResizeDynamicLayout()
{
    // m_pDynamicLayout->Adjust() will update the control position regardless of window scrolling
    // Previously, the reset after OnSize of the parent window was unreliable (it did not work properly when entering, exiting, maximizing, or snapping), and I did not find a way to check out these two special OnSizes.
    // It is too early to call ResizeDynamicLayout inside the framework, and there is no time to reset the scrolling first, so overwrite this virtual method to intervene.

    // If scrolling is enabled, then the base class is not called here to invalidate the processing of the frame. Instead, it is called by itself after the OnSize of the parent window.
    if (!m_scroll_enable)
        CBaseDialog::ResizeDynamicLayout();
}

void CTabDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    // TODO: Add message handler code and/or call defaults here
    //Reference: https://www.cnblogs.com/ranjiewen/p/6013922.html

    if (m_scroll_enable)
    {
        int step = theApp.DPI(16);
        switch (nSBCode)
        {
        case SB_LINEUP:                         //Scroll one line up
            ScrollWindowSimple(step);
            break;
        case SB_LINEDOWN:                       //Scroll one line down
            ScrollWindowSimple(-step);
            break;
        case SB_PAGEUP:                         //Scroll one page up.
            ScrollWindowSimple(INT_MAX);
            break;
        case SB_PAGEDOWN:                       //Scroll one page down
            ScrollWindowSimple(INT_MIN);
            break;
        case SB_THUMBPOSITION:                  //Scroll to the absolute position. The current position is provided in nPos
            break;
        case SB_THUMBTRACK:                     //Drag scroll box to specified position. The current position is provided in nPos
            ScrollWindowSimple(nPos, true);
            break;
        case SB_TOP:                            // top
            ScrollWindowSimple(0, true);
            break;
        case SB_BOTTOM:                         // bottom
            ScrollWindowSimple(-1, true);
            break;
        case SB_ENDSCROLL:                      //End scroll
            break;
        default:
            break;
        }
    }
    CBaseDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}


BOOL CTabDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    // TODO: Add message handler code and/or call defaults here
    if (m_scroll_enable)
    {
        int step = theApp.DPI(64) * zDelta / 120;
        ScrollWindowSimple(step);
        SCROLLINFO scrollinfo;
        GetScrollInfo(SB_VERT, &scrollinfo, SIF_ALL);
        m_last_pos = scrollinfo.nPos;
    }

    return CBaseDialog::OnMouseWheel(nFlags, zDelta, pt);
}


void CTabDlg::OnOK()
{
    // TODO: Add specialized code here and/or call the base class

    CBaseDialog::OnOK();
}


bool CTabDlg::IsScrollBarVisible()
{
    SCROLLINFO scrollinfo;
    GetScrollInfo(SB_VERT, &scrollinfo, SIF_ALL);
    bool visible = scrollinfo.nPage < scrollinfo.nMax;
    return visible;
}

void CTabDlg::OnSize(UINT nType, int cx, int cy)
{
    CBaseDialog::OnSize(nType, cx, cy);

    // TODO: add message handler code here
    SetControlMouseWheelEnable(!IsScrollBarVisible());      //If scroll bars are displayed, disables the control from responding to the mouse wheel, which is used to scroll the window
}
