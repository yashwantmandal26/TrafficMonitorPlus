// SpinEdit.cpp: implementation file
//

#include "stdafx.h"
#include "SpinEdit.h"

#define SPIN_ID 3100
// CSpinEdit

IMPLEMENT_DYNAMIC(CSpinEdit, CEdit)

CSpinEdit::CSpinEdit()
{

}

CSpinEdit::~CSpinEdit()
{
}

void CSpinEdit::SetRange(short lower, short upper, short step)
{
    m_step = step;
    if (m_spin.GetSafeHwnd() != NULL)
        m_spin.SetRange(lower, upper);
}

void CSpinEdit::SetValue(int value)
{
    ASSERT(m_step != 0);    // Range should be set first
    if (m_spin.GetSafeHwnd() != NULL)
        m_spin.SetPos(value);
}

int CSpinEdit::GetValue()
{
    if (m_spin.GetSafeHwnd() != NULL)
        return m_spin.GetPos32();
    else
        return 0;
}


void CSpinEdit::SetMouseWheelEnable(bool enable)
{
    m_mouse_wheel_enable = enable;
}


BEGIN_MESSAGE_MAP(CSpinEdit, CEdit)
    ON_MESSAGE(WM_TABLET_QUERYSYSTEMGESTURESTATUS, &CSpinEdit::OnTabletQuerysystemgesturestatus)
    ON_NOTIFY(UDN_DELTAPOS, SPIN_ID, &CSpinEdit::OnDeltaposSpin)
    ON_WM_KILLFOCUS()
    ON_WM_ENABLE()
    ON_WM_SIZE()
END_MESSAGE_MAP()



// CSpinEdit message handler

void CSpinEdit::PreSubclassWindow()
{
    // TODO: Add specialized code here and/or call the base class

    // The text is required to be aligned to the left, and the right side will be covered by the spin control.
    // WS_CLIPCHILDREN excludes sub-window areas during drawing to avoid overlapping controls drawing flickering
    ModifyStyle(ES_CENTER | ES_RIGHT, ES_LEFT | WS_CLIPCHILDREN);
    m_spin.Create(WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_HOTTRACK | UDS_SETBUDDYINT, CRect(), this, SPIN_ID);
    // Get the width of the spin control
    CRect rect;
    m_spin.GetWindowRect(&rect);
    m_spin_width = rect.Width();
    // Set the spin control area (as a child control of CEdit, the part outside the ClientRect area will be cropped)
    GetClientRect(&rect);
    rect.left = rect.right - m_spin_width;
    m_spin.MoveWindow(&rect);
    // Set the Edit control as an associated control
    m_spin.SetBuddy(this);
    // Set default scope
    m_spin.SetRange(0, 999);
    CEdit::PreSubclassWindow();
}


BOOL CSpinEdit::PreTranslateMessage(MSG* pMsg)
{
    // TODO: Add specialized code here and/or call the base class

    //If m_mouse_wheel_enable is false, mouse wheel messages will not be responded to
    if (pMsg->message == WM_MOUSEWHEEL && !m_mouse_wheel_enable)
    {
        //Send mouse wheel message to parent window
        CWnd* pParent = GetParent();
        pParent->SendMessage(WM_MOUSEWHEEL, pMsg->wParam, pMsg->lParam);
        return true;
    }

    return CEdit::PreTranslateMessage(pMsg);
}


afx_msg LRESULT CSpinEdit::OnTabletQuerysystemgesturestatus(WPARAM wParam, LPARAM lParam)
{
    return 0;
}


void CSpinEdit::OnDeltaposSpin(NMHDR* pNMHDR, LRESULT* pResult)
{
    //This responds to events when the spin button clicks the up and down buttons.
    LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

    int value = GetValue();
    value += pNMUpDown->iDelta * m_step;
    value /= m_step;
    value *= m_step;
    SetValue(value);
    pNMUpDown->iDelta = 0;
    *pResult = 0;
    CWnd* pParent = GetParent();
    if (pParent != nullptr)
        pParent->SendMessage(WM_SPIN_EDIT_POS_CHANGED, (WPARAM)this, (LPARAM)pNMUpDown);
}


void CSpinEdit::OnKillFocus(CWnd* pNewWnd)
{
    CEdit::OnKillFocus(pNewWnd);

    // TODO: add message handler code here
    CString str;
    GetWindowTextW(str);
    int value = _ttoi(str.GetString());
    SetValue(value);    // m_spin will check and limit the value range and update the CEdit control text
}


void CSpinEdit::OnEnable(BOOL bEnable)
{
    CEdit::OnEnable(bEnable);
    m_spin.EnableWindow(bEnable);
}


void CSpinEdit::OnSize(UINT nType, int cx, int cy)
{
    CEdit::OnSize(nType, cx, cy);
    CRect rect;
    GetClientRect(&rect);
    rect.left = rect.right - m_spin_width;
    m_spin.MoveWindow(&rect);
}
