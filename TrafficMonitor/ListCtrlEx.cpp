#include "stdafx.h"
#include "ListCtrlEx.h"

IMPLEMENT_DYNAMIC(CListCtrlEx, CListCtrl)

CListCtrlEx::CListCtrlEx()
{
}


CListCtrlEx::~CListCtrlEx()
{
}

void CListCtrlEx::Edit(int row, int col)
{
    EnsureVisible(row, FALSE);				//Make sure a row is visible when editing it
    m_editing = true;

    m_edit_row = row;
    m_edit_col = col;
    CRect item_rect;
    GetSubItemRect(row, col, LVIR_LABEL, item_rect);	//Get the rectangle of the child

    CString text = GetItemText(row, col);		//Get the content of the child item

    m_item_edit.SetWindowText(text);		//Display the content of the subitem into the edit box
    m_item_edit.ShowWindow(SW_SHOW);		//Show edit box
    m_item_edit.MoveWindow(item_rect);		//Move the edit box above the child item and cover it
    m_item_edit.SetFocus();					//Focus the edit box
    m_item_edit.SetSel(0, -1);
}

void CListCtrlEx::SetEditColMethod(eEditColMethod method)
{
    m_edit_col_method = method;
}

void CListCtrlEx::SetEditableCol(const std::initializer_list<int>& paras)
{
    m_edit_cols = paras;
}

void CListCtrlEx::EndEdit()
{
    if (m_editing)
    {
        if (m_edit_row >= 0 && m_edit_row < GetItemCount())
        {
            CString str;
            m_item_edit.GetWindowText(str);	//Get the contents of the edit box
            SetItemText(m_edit_row, m_edit_col, str);	//Update the content into CListCtrl
        }
        m_item_edit.ShowWindow(SW_HIDE);//Hide edit box
        m_editing = false;
    }
}

BEGIN_MESSAGE_MAP(CListCtrlEx, CListCtrl)
    ON_EN_KILLFOCUS(IDC_ITEM_EDITBOX, &CListCtrlEx::OnEnKillfocusEdit1)
    ON_NOTIFY_REFLECT_EX(NM_DBLCLK, &CListCtrlEx::OnNMDblclk)
    ON_NOTIFY_REFLECT(LVN_BEGINSCROLL, &CListCtrlEx::OnLvnBeginScroll)
    ON_MESSAGE(WM_TABLET_QUERYSYSTEMGESTURESTATUS, &CListCtrlEx::OnTabletQuerysystemgesturestatus)
END_MESSAGE_MAP()


void CListCtrlEx::OnEnKillfocusEdit1()
{
    //Responding when a text edit control control loses focus
    EndEdit();
}



void CListCtrlEx::PreSubclassWindow()
{
    // TODO: Add specialized code here and/or call the base class
    m_item_edit.Create(WS_BORDER | ES_AUTOHSCROLL, CRect(), this, IDC_ITEM_EDITBOX);
    m_item_edit.SetFont(GetFont());

    CListCtrl::PreSubclassWindow();
}


BOOL CListCtrlEx::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: Add control notification handler code here

    //If the double-clicked column is the column to be edited
    if (m_edit_col_method == EC_ALL
        || (m_edit_col_method == EC_SPECIFIED && m_edit_cols.find(pNMItemActivate->iSubItem) != m_edit_cols.end()))
    {
        Edit(pNMItemActivate->iItem, pNMItemActivate->iSubItem);
        *pResult = 0;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


void CListCtrlEx::OnLvnBeginScroll(NMHDR *pNMHDR, LRESULT *pResult)
{
    // This feature requires Internet Explorer 5.5 or higher.
    // Symbol _WIN32_IE must be >= 0x0560.
    LPNMLVSCROLL pStateChanged = reinterpret_cast<LPNMLVSCROLL>(pNMHDR);
    // TODO: Add control notification handler code here
    EndEdit();

    *pResult = 0;
}


afx_msg LRESULT CListCtrlEx::OnTabletQuerysystemgesturestatus(WPARAM wParam, LPARAM lParam)
{
    return 0;
}


BOOL CListCtrlEx::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
    {
        //Press the Enter key in editing mode to end editing instead of closing the dialog box
        if (m_editing)
        {
            EndEdit();
            return TRUE;
        }
    }

    return CListCtrl::PreTranslateMessage(pMsg);
}
