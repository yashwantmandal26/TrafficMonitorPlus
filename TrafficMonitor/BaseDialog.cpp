// BaseDialog.cpp: implementation file
//

#include "stdafx.h"
#include "BaseDialog.h"
#include "afxdialogex.h"
#include "IniHelper.h"
#include "TrafficMonitor.h"
#include "TrafficMonitorDlg.h"

// CBaseDialog dialog box
std::map<CString, HWND> CBaseDialog::m_unique_hwnd;

IMPLEMENT_DYNAMIC(CBaseDialog, CDialog)

CBaseDialog::CBaseDialog(UINT nIDTemplate, CWnd* pParent /*=NULL*/)
    : CDialog(nIDTemplate, pParent)
{
    m_nDialogID = nIDTemplate;
    m_dpi = theApp.GetDpi();
}

CBaseDialog::~CBaseDialog()
{
}

void CBaseDialog::SetBackgroundColor(COLORREF color, BOOL bRepaint)
{
    if (m_brBkgr.GetSafeHandle() != NULL)
    {
        m_brBkgr.DeleteObject();
    }

    if (color != (COLORREF)-1)
    {
        m_brBkgr.CreateSolidBrush(color);
    }

    if (bRepaint && GetSafeHwnd() != NULL)
    {
        Invalidate();
        UpdateWindow();
    }
}

void CBaseDialog::SetMinSize(int cx, int cy)
{
    m_min_size.cx = cx;
    m_min_size.cy = cy;
}

HWND CBaseDialog::GetUniqueHandel(LPCTSTR dlg_name)
{
    return m_unique_hwnd[dlg_name];
}

const std::map<CString, HWND>& CBaseDialog::AllUniqueHandels()
{
    return m_unique_hwnd;
}

bool CBaseDialog::IsAllDialogClosed()
{
    for (const auto& hwnd : m_unique_hwnd)
    {
        if (hwnd.second != nullptr)
            return false;
    }
    return true;
}

void CBaseDialog::LoadConfig()
{
    if (!GetDialogName().IsEmpty() && m_remember_dlg_size)
    {
        CIniHelper ini{ theApp.m_config_path };
        //Loading window size settings
        m_window_size.cx = ini.GetInt(_T("window_size"), GetDialogName() + _T("_width"), -1);
        m_window_size.cy = ini.GetInt(_T("window_size"), GetDialogName() + _T("_height"), -1);
    }
}

void CBaseDialog::SaveConfig() const
{
    if (!GetDialogName().IsEmpty() && m_remember_dlg_size)
    {
        CIniHelper ini{ theApp.m_config_path };
        //Save window size settings
        ini.WriteInt(_T("window_size"), GetDialogName() + _T("_width"), m_window_size.cx);
        ini.WriteInt(_T("window_size"), GetDialogName() + _T("_height"), m_window_size.cy);
        ini.Save();
    }
}

void CBaseDialog::IterateControls(CWnd* pParent, std::function<void(CWnd*)> func)
{
    if (pParent == nullptr)
        return;

    // Get the first child control
    CWnd* pChild = pParent->GetWindow(GW_CHILD);

    // Traverse all child controls
    while (pChild != nullptr)
    {
        func(pChild);

        // Recursively traverse child controls of child controls (handling nested controls)
        IterateControls(pChild, func);

        // Get the next sibling control
        pChild = pChild->GetWindow(GW_HWNDNEXT);
    }
}

void CBaseDialog::ReLoadLayoutResource()
{
    ASSERT(m_nDialogID);
    CMFCDynamicLayout* pDynamicLayout = GetDynamicLayout();
    if (pDynamicLayout)
    {
        HRSRC layoutRes = ::FindResourceW(NULL, MAKEINTRESOURCEW(m_nDialogID), RT_DIALOG_LAYOUT);
        if (layoutRes)
        {
            HGLOBAL hResData = ::LoadResource(NULL, layoutRes);
            if (hResData)
            {
                LPVOID layoutResData = ::LockResource(hResData);
                DWORD layoutResSize = ::SizeofResource(NULL, layoutRes);
                // std::wstring data(static_cast<wchar_t*>(layoutResData), layoutResSize / sizeof(wchar_t));
                pDynamicLayout->LoadResource(this, layoutResData, layoutResSize);
            }
        }
    }
}

int CBaseDialog::DPI(int pixel) const
{
    return m_dpi * pixel / 96;
}

CRect CBaseDialog::GetTextExtent(const CString& text)
{
    ASSERT(m_pDC != nullptr);   // m_pDC is responsible for applying for release by OnInitDialog
    if (m_pDC == nullptr)
        return CRect();
    if (text.IsEmpty())
        return CRect();
    CRect text_size;
    m_pDC->DrawTextW(text, &text_size, DT_CALCRECT);    // Use CDC::DrawTextW to measure text width (CDC::GetTextExtent is the theoretical width and is not accurate)
    return text_size;
}

void CBaseDialog::RepositionTextBasedControls(const vector<CtrlTextInfo>& items, CtrlTextInfo::Width center_min_width)
{
    ASSERT(m_pDC != nullptr);   // This method is only available during InitializeControls
    if (m_pDC == nullptr)
        return;
    int center_width = theApp.DPI(center_min_width);
    std::map<int, std::pair<int, int>> col_info;
    struct itemInfo
    {
        int col_index;
        CWnd* p;
        CRect rect;
    };
    vector<itemInfo> items_info;
    int center_left{ 0 }, center_right{ INT_MAX };

    for (const auto& item : items)
    {
        ASSERT(item.col_index != CtrlTextInfo::UN_USE);
        ASSERT(item.id != 0);
        // Get the required dx for all columns, as well as the right edge center_left of the left-aligned element, and the left edge center_right of the right-aligned element.
        CWnd* pItem = GetDlgItem(item.id);
        if (pItem == nullptr)
            continue;
        CRect rect{};
        pItem->GetWindowRect(&rect);
        ScreenToClient(&rect);
        CString text;
        pItem->GetWindowTextW(text);
        int dx = GetTextExtent(text).Width() + theApp.DPI(item.ext_width) - rect.Width();
        if (dx < 0) dx = 0;                                     // Text only increases the width of the control
        if (col_info[item.col_index].first < dx)                // Get the element in this column whose width has increased the most
            col_info[item.col_index].first = dx;
        if (item.col_index < 0 && center_left < rect.right)
            center_left = rect.right;
        if (item.col_index > 0 && center_right > rect.left)
            center_right = rect.left;
        if (item.col_index == 0)    // There may be multiple controls with col_index 0
        {
            if (center_left < rect.left)
                center_left = rect.left;
            if (center_right > rect.right)
                center_right = rect.right;
        }
        items_info.emplace_back(itemInfo{ item.col_index, pItem, std::move(rect) });
    }

    if (center_right == INT_MAX)    // If all controls are left-aligned, then the right edge of the window is used as the right edge of the remaining space.
    {
        CRect dlg_rect{};
        GetClientRect(&dlg_rect);
        center_right = dlg_rect.Width();
    }
    // This assertion trigger indicates that the original layout in the resource file did not leave enough width for the middle control/free space
    ASSERT(center_right - center_left >= center_width);
    int dx_sum_left{}, dx_sum_right{};
    // Because the same col_index can have multiple controls & no order is required, the final position of the control must be calculated statelessly.
    for (auto& a : col_info)
    {
        if (a.first < 0)
        {
            a.second.second = dx_sum_left;    // Store the total dx of the control before this column, that is, the right movement distance of the control in this column
            dx_sum_left += a.second.first;
        }
        else if (a.first > 0)
        {
            a.second.second = dx_sum_right;
            dx_sum_right += a.second.first;
        }
    }
    float scale{ 1.0f };
    if (center_right - center_left - dx_sum_left - dx_sum_right < center_width)
    {
        // ASSERT(false);
        // The text is now loaded so that the middle control/free space of this row is squeezed too small
        // This requires redesigning the window control arrangement to adapt to the current translation length. Here, we simply allocate the missing space to each dx.
        scale = static_cast<float>(center_right - center_left - center_width) / (dx_sum_left + dx_sum_right);
        dx_sum_left = static_cast<int>(dx_sum_left * scale + 0.5f);
        dx_sum_right = static_cast<int>(dx_sum_right * scale + 0.5f);
    }
    for (const auto& item : items_info)
    {
        const auto& rect = item.rect;
        int dx = static_cast<int>(col_info[item.col_index].first * scale + 0.5f);
        int sum_dx = static_cast<int>(col_info[item.col_index].second * scale + 0.5f);
        if (item.col_index < 0)
            item.p->SetWindowPos(nullptr, rect.left + sum_dx, rect.top, rect.Width() + dx, rect.Height(), SWP_NOZORDER);
        else if (item.col_index > 0)
            item.p->SetWindowPos(nullptr, rect.left + sum_dx - dx_sum_right, rect.top, rect.Width() + dx, rect.Height(), SWP_NOZORDER);
        else if (item.col_index == 0)
            item.p->SetWindowPos(nullptr, rect.left + dx_sum_left, rect.top, rect.Width() - dx_sum_left - dx_sum_right, rect.Height(), SWP_NOZORDER);
    }
}

void CBaseDialog::EnableDlgCtrl(UINT id, bool enable)
{
    CWnd* pWnd = GetDlgItem(id);
    if (pWnd != nullptr)
        pWnd->EnableWindow(enable);
}

void CBaseDialog::SetButtonIcon(UINT id, HICON hIcon)
{
    CWnd* dlgItem = GetDlgItem(id);
    CButton* btn = static_cast<CButton*>(dlgItem);
    if (btn != nullptr)
        btn->SetIcon(hIcon);
}

CRect CBaseDialog::GetControlRect(CWnd* pCtrl)
{
    if (pCtrl != nullptr)
    {
        CRect rect;
        pCtrl->GetWindowRect(rect);
        ScreenToClient(rect);
        return rect;
    }
    return CRect();
}

CRect CBaseDialog::GetControlRect(UINT id)
{
    CWnd* pCtrl = GetDlgItem(id);
    return GetControlRect(pCtrl);
}

void CBaseDialog::IterateControls(std::function<void(CWnd*)> func)
{
    func(this);
    IterateControls(this, func);
}

void CBaseDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CBaseDialog, CDialog)
    ON_WM_DESTROY()
    ON_WM_GETMINMAXINFO()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CBaseDialog message handler


BOOL CBaseDialog::OnInitDialog()
{
    m_unique_hwnd[GetDialogName()] = m_hWnd;
    CDialog::OnInitDialog();

    // TODO: Add additional initialization here

    //Initialize font
    CRect rect;
    GetWindowRect(rect);
    FontInfo font_info;
    font_info.name = theApp.m_str_table.GetLanguageInfo().default_font_name.c_str();
    font_info.size = 9;
    UINT dpi_x{}, dpi_y{};
    if (theApp.DPIFromRect(rect, &dpi_x, &dpi_y))
        m_dpi = dpi_x;

    font_info.Create(m_dlg_font, m_dpi);

    //Get the initial window size
    if (m_min_size.cx <= 0 || m_min_size.cy <= 0)
    {
        CRect rect;
        GetWindowRect(rect);
        m_min_size.cx = rect.Width() * 96 / theApp.GetDpi();
        m_min_size.cy = rect.Height() * 96 / theApp.GetDpi();
    }

    //Load settings
    LoadConfig();

    SetWindowPos(&wndNoTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); //Cancel top

    //Handle text translation in dialog boxes 
    IterateControls([&](CWnd* pWnd) {
        //Set control font
        if (m_dlg_font.GetSafeHandle() != NULL)
            pWnd->SetFont(&m_dlg_font);
        //Get control text
        CString str;
        pWnd->GetWindowText(str);
        UINT id = pWnd->GetDlgCtrlID();
        if (str.Left(4) == _T("TXT_"))
        {
            //Set control text
            const std::wstring& str_translated = theApp.m_str_table.LoadText(str.GetString());
            if (!str_translated.empty())
                pWnd->SetWindowTextW(str_translated.c_str());
        }
        //Handle standard buttons
        else
        {
            if (id == IDOK)
                pWnd->SetWindowTextW(theApp.m_str_table.LoadText(TXT_OK).c_str());
            else if (id == IDCANCEL)
                pWnd->SetWindowTextW(theApp.m_str_table.LoadText(TXT_CANCEL).c_str());
            else if (id == IDCLOSE)
                pWnd->SetWindowTextW(theApp.m_str_table.LoadText(TXT_CLOSE).c_str());
        }
    });

    // Before restoring the window size (the current window status is consistent with the resource), the derived class performs control text initialization and adjusts the control arrangement.
    // Initialization related to the actual window size (such as table column width) should be performed in OnInitDialog of the derived class
    m_pDC = GetDC();
    m_pDC->SelectObject(&m_dlg_font);
    bool rtn = InitializeControls();
    ReleaseDC(m_pDC);
    m_pDC = nullptr;
    // If the control layout has been changed then true should be returned to apply the control adjustments to the layout manager (reloading dynamic layout settings)
    if (rtn)
        ReLoadLayoutResource();


    //Initialize window size
    if (m_window_size.cx > 0 && m_window_size.cy > 0)
    {
        if (m_min_size.cx > 0 && m_window_size.cx < theApp.DPI(m_min_size.cx))
            m_window_size.cx = theApp.DPI(m_min_size.cx);
        if (m_min_size.cy > 0 && m_window_size.cy < theApp.DPI(m_min_size.cy))
            m_window_size.cy = theApp.DPI(m_min_size.cy);
        SetWindowPos(nullptr, 0, 0, m_window_size.cx, m_window_size.cy, SWP_NOZORDER | SWP_NOMOVE);
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // Exception: OCX property page should return FALSE
}


void CBaseDialog::OnDestroy()
{
    CDialog::OnDestroy();

    // TODO: add message handler code here
    m_unique_hwnd[GetDialogName()] = NULL;
    SaveConfig();

    //Reset the main window to the top when all dialog boxes are closed
    CTrafficMonitorDlg* pDlg = dynamic_cast<CTrafficMonitorDlg*>(theApp.m_pMainWnd);
    if (pDlg != nullptr && IsAllDialogClosed())
        pDlg->SetAlwaysOnTop();
}


void CBaseDialog::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    // TODO: Add message handler code and/or call defaults here
    //Limit window minimum size
    lpMMI->ptMinTrackSize.x = theApp.DPI(m_min_size.cx);		//Set minimum width
    lpMMI->ptMinTrackSize.y = theApp.DPI(m_min_size.cy);		//Set minimum height

    CDialog::OnGetMinMaxInfo(lpMMI);
}


void CBaseDialog::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    // TODO: add message handler code here
    if (nType != SIZE_MAXIMIZED && nType != SIZE_MINIMIZED)
    {
        //m_window_width = cx;
        //m_window_hight = cy;
        CRect rect;
        GetWindowRect(&rect);
        m_window_size.cx = rect.Width();
        m_window_size.cy = rect.Height();
    }

}


INT_PTR CBaseDialog::DoModal()
{
    HWND unique_hwnd{ m_unique_hwnd[GetDialogName()] };
    if (unique_hwnd != NULL && !GetDialogName().IsEmpty())      ///If the dialog box already exists, display the existing dialog box
    {
        ::ShowWindow(unique_hwnd, SW_RESTORE);
        ::SetForegroundWindow(unique_hwnd);
        return 0;
    }
    return CDialog::DoModal();
}


BOOL CBaseDialog::OnEraseBkgnd(CDC* pDC)
{
    // Modify window background (CDialogEx)
    if (m_brBkgr.GetSafeHandle() != NULL)
    {
        ASSERT_VALID(pDC);
        CRect rectClient;
        GetClientRect(rectClient);
        pDC->FillRect(rectClient, &m_brBkgr);
        return TRUE;
    }
    return CDialog::OnEraseBkgnd(pDC);
}


HBRUSH CBaseDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    // Modify window background (CDialogEx)
    if (m_brBkgr.GetSafeHandle() != NULL)
    {
#define AFX_MAX_CLASS_NAME 255
#define AFX_STATIC_CLASS _T("Static")
#define AFX_BUTTON_CLASS _T("Button")
#define AFX_SLIDER_CLASS _T("msctls_trackbar32")    // Slider control CSliderCtrl and its derived classes
#define AFX_SYSLINK_CLASS _T("SysLink")             // Hyperlink control CSysLink and its derived classes

        if (nCtlColor == CTLCOLOR_STATIC)
        {
            TCHAR lpszClassName[AFX_MAX_CLASS_NAME + 1];

            ::GetClassName(pWnd->GetSafeHwnd(), lpszClassName, AFX_MAX_CLASS_NAME);
            CString strClass = lpszClassName;

            if (strClass == AFX_BUTTON_CLASS || strClass == AFX_STATIC_CLASS || strClass == AFX_SLIDER_CLASS || strClass == AFX_SYSLINK_CLASS)
            {
                pDC->SetBkMode(TRANSPARENT);

                if (m_brBkgr.GetSafeHandle() != NULL && IsAppThemed())
                {
                    return (HBRUSH)m_brBkgr.GetSafeHandle();
                }
                else
                {
                    return (HBRUSH)::GetStockObject(HOLLOW_BRUSH);
                }
            }
        }
    }

    return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}
