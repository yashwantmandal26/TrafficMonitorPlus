#pragma once
// This is an Edit control with a spin button. A spin button is attached to the right side of CEdit. Please reserve space.
// CSpinEdit

//Spinner button click message. WPARAM: pointer to CSpinButtonCtrl, LPARAM: LPNMUPDOWN
#define WM_SPIN_EDIT_POS_CHANGED (WM_USER + 1010)

class CSpinEdit : public CEdit
{
    DECLARE_DYNAMIC(CSpinEdit)

public:
    CSpinEdit();
    virtual ~CSpinEdit();

    void SetRange(short lower, short upper, short step = 1);        // Set the range of values in the text box
    void SetValue(int value);                       //Set the value in the text box
    int GetValue();                                 //Get the value in the text box
    void SetMouseWheelEnable(bool enable);          //Set whether to allow response to the mouse wheel

protected:
    CSpinButtonCtrl m_spin;                         //Spin button control
    int m_step{ 1 };
    int m_spin_width{};
    bool m_mouse_wheel_enable{ true };

    DECLARE_MESSAGE_MAP()
    virtual void PreSubclassWindow();
public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
    afx_msg LRESULT OnTabletQuerysystemgesturestatus(WPARAM wParam, LPARAM lParam);
public:
    void OnDeltaposSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnEnable(BOOL bEnable);
    afx_msg void OnSize(UINT nType, int cx, int cy);
};


