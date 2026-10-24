// ComboBox2.cpp: implementation file
//

#include "stdafx.h"
#include "ComboBox2.h"


// CComboBox2

IMPLEMENT_DYNAMIC(CComboBox2, CComboBox)

CComboBox2::CComboBox2()
{

}

CComboBox2::~CComboBox2()
{
}


void CComboBox2::SetMouseWheelEnable(bool enable)
{
    m_mouse_wheel_enable = enable;
}

BEGIN_MESSAGE_MAP(CComboBox2, CComboBox)
END_MESSAGE_MAP()



// CComboBox2 message handler




BOOL CComboBox2::PreTranslateMessage(MSG* pMsg)
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

    return CComboBox::PreTranslateMessage(pMsg);
}
