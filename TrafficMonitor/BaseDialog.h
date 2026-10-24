#pragma once
#include <functional>

// CBaseDialog dialog box
//Used to implement remembering the dialog box size
//and set the window initial size to the minimum size

class CBaseDialog : public CDialog
{
    DECLARE_DYNAMIC(CBaseDialog)

public:
    CBaseDialog(UINT nIDTemplate, CWnd* pParent = NULL);   // Standard constructor
    virtual ~CBaseDialog();

    // Copied from CDialogEx and has the same function (added processing of slider control and hyperlink control)
    void SetBackgroundColor(COLORREF color, BOOL bRepaint = TRUE);

    // Dialog data
    //#ifdef AFX_DESIGN_TIME
    //	enum { IDD = IDD_BASEDIALOG };
    //#endif

    void SetMinSize(int cx, int cy);		//Set the minimum size of the window, if not set, use the initial size of the window as the minimum size
    static HWND GetUniqueHandel(LPCTSTR dlg_name);          //Get the unique handle of the specified window
    static const std::map<CString, HWND>& AllUniqueHandels();   //Get handles of all windows
    static bool IsAllDialogClosed();

private:
    void LoadConfig();
    void SaveConfig() const;
    void IterateControls(CWnd* pParent, std::function<void(CWnd*)> func);
    // Reapply the layout manager parameters, which will cause the control's base size/position to be based on the current one
    void ReLoadLayoutResource();

private:
    UINT m_nDialogID;                                   // Member variables are used to store resource IDs
    CSize m_min_size{};     //Minimum size of window (saved at 96dpi)
    CSize m_window_size{ -1, -1 };
    CBrush m_brBkgr;
    CDC* m_pDC = nullptr;                               // Valid during InitializeControls, used to measure text length
    static std::map<CString, HWND> m_unique_hwnd;       //Unique window handle for each base class
    bool m_remember_dlg_size{ true };                   //Whether to remember the window size (the window size will be remembered when this flag is true and the string returned by GetDialogName is not empty)
    CFont m_dlg_font;
    int m_dpi;              //Window's own DPI

protected:
    int DPI(int pixel) const;

    // Only available during InitializeControls, measures control text length
    CRect GetTextExtent(const CString& text);

    struct CtrlTextInfo
    {
        // The enumeration here is used as a type-checkable int
        // The specific value of Col is meaningless. It only indicates the relative order between columns and which controls are in the same column. In fact, there is no limit on the number. You can increase it if necessary.
        enum Col { L4 = -4, L3 = -3, L2 = -2, L1 = -1, C0 = 0, R1 = 1, R2 = 2, R3 = 3, R4 = 4, UN_USE = 100 };
        // The remaining width of the control after removing the text.
        // W0 is recommended for static text, and W32 is recommended for buttons because they may have icons.
        enum Width { W_50 = -50, W0 = 0, W16 = 16, W24 = 24, W32 = 32, W40 = 40, W60 = 60, W64 = 64, W96 = 96, W128 = 128, W256 = 256 };

        Col col_index{ UN_USE };    // Indicates the position of the control, increasing from left to right. If it is less than 0, it will stick to the left. If it is greater than 0, it will stick to the right. If it is equal to 0, it will use the remaining space.
        UINT id{ 0 };               // Control ID
        Width ext_width{ W0 };      // The width of the control needs to be at least "text width + ext_width" (DPI conversion theApp.DPI(ext_width) will be performed internally)
    };
    // Only available during InitializeControls. Rearranges controls according to text length. No vertical adjustments are made and control spacing is not changed.
    // It will only increase the width of the control, so it is recommended to set a smaller width in the resource and use this method to adjust it to the appropriate state.
    // Prioritize to ensure that the remaining width in the middle (or the width of the control with col_index 0) is at least center_min_width
    // When there is insufficient space, the text of other controls will not be fully displayed (the window should be redesigned at this time) (this method is only applicable when the text cannot be too long)
    void RepositionTextBasedControls(const vector<CtrlTextInfo>& items, CtrlTextInfo::Width center_min_width = CtrlTextInfo::W16);
    
    // Called by CBaseDialog::OnInitDialog before restoring the window size in the configuration
    // Derived classes perform some control initialization, such as setting control text and rearranging controls RepositionTextBasedControls
    // Returning true will re-apply the layout manager parameters, which will make the base size/position of the control's dynamic layout manager based on the current one.
    // Initialization related to the actual window size (such as table column width) should be performed in OnInitDialog of the derived class
    virtual bool InitializeControls() { return false; };

    virtual CString GetDialogName() const { return CString(); }
    void EnableDlgCtrl(UINT id, bool enable);
    void SetButtonIcon(UINT id, HICON hIcon);
    void SetRememberDlgSize(bool enable) { m_remember_dlg_size = enable; }
    CRect GetControlRect(CWnd* pCtrl);
    CRect GetControlRect(UINT id);

    //Traverse all child controls
    void IterateControls(std::function<void(CWnd*)> func);

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    virtual INT_PTR DoModal();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
