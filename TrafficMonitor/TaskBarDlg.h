#pragma once
#include "Common.h"
#include "afxwin.h"
#include "DrawCommon.h"
#include "TaskBarDlgDrawCommon.h"
#include "TrafficMonitor.h"
#include "IniHelper.h"
#include "CommonData.h"
#include "TaskbarItemOrderHelper.h"
#include "SupportedRenderEnums.h"
#include <list>

// CTaskBarDlg dialog box
#define TASKBAR_WND_HEIGHT DPI(32)				//The height of the taskbar window
#define WM_TASKBAR_MENU_POPED_UP (WM_USER + 1004)		//Define the message sent when the right-click menu of the taskbar window pops up
//#define TASKBAR_GRAPH_MAX_LEN 600 //Maximum length of historical data storage
#define TASKBAR_GRAPH_STEP 5							//Draw a line in seconds

class CTaskBarDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CTaskBarDlg)

public:
    CTaskBarDlg(CWnd* pParent = NULL);   // Standard constructor
    virtual ~CTaskBarDlg();

    CToolTipCtrl m_tool_tips;

    void ShowInfo(CDC* pDC); 	//Draw information onto the control
    void TryDrawStatusBar(IDrawCommon& drawer, const CRect& rect_bar, int usage_percent); //Draw CPU/memory status bar

    void TryDrawGraph(IDrawCommon& drawer, const CRect& value_rect, CommonDisplayItem item_type);		// Plot CPU/memory dynamics

    bool AdjustWindowPos(bool force_adjust = false);	//Set the position of the window in the taskbar (if force_adjust is true, the position of the taskbar window will be forcibly adjusted once)
    void ApplyWindowTransparentColor();

    //bool IsTaskbarChanged();

    void WidthChanged();    //Call this function to notify the taskbar window width change to force the taskbar window position to be adjusted once

    //Get the rectangular area used to check DPI
    const CRect& GetRectForDpiCheck() const;

    UINT GetDPI() const;
    void SetDPI(UINT dpi);
    UINT DPI(UINT pixel) const;
    int DPI(int pixel) const;
    LONG DPI(LONG pixel) const;
    void DPI(CRect& rect) const;

    //The following class definition can be regarded as a function definition to avoid different static variables caused by different instantiations of template functions.
    // Approximate function declaration
    // template <class HanlderFunc>
    // CheckWindowMonitorDPIAndHandle(CTaskBarDlg& ref_taskbar_window, HanlderFunc handler)
    static class ClassCheckWindowMonitorDPIAndHandle
    {
    public:
        template <class HandlerFunc>
        void operator()(CTaskBarDlg& ref_taskbar_window, HandlerFunc handler)
        {
            bool rtn = theApp.DPIFromRect(ref_taskbar_window.GetRectForDpiCheck(), &dpi_x, &dpi_y);
            //Only take dpi_x as program dpi
            if (rtn && (dpi_x != buffered_dpi_x || dpi_y != buffered_dpi_y))
            {
                //Update cached data
                buffered_dpi_x = dpi_x;
                buffered_dpi_y = dpi_y;
                //Call user-defined processing method
                handler(dpi_x, dpi_y);
            }
        }

    private:
        static UINT buffered_dpi_x, buffered_dpi_y;
        static UINT dpi_x, dpi_y;
    } CheckWindowMonitorDPIAndHandle;

    // Dialog data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_TASK_BAR_DIALOG };
#endif

protected:
    virtual void InitTaskbarWnd() = 0;
    virtual void AdjustTaskbarWndPos(bool force_adjust) = 0;
    virtual void ResetTaskbarPos() = 0;
    virtual void CheckTaskbarOnTopOrBottom() = 0;		//Check if the taskbar is at the top or bottom of the screen and save the result in m_taskbar_on_top_or_bottom
    virtual HWND GetParentHwnd() = 0;

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    HWND m_hTaskbar;	//taskbar window handle
    CRect m_rcTaskbar;  //The rectangular area of the taskbar
    CRect m_rect;		//The rectangular area of the current window
    int m_window_width{};   //Save calculated window width
    int m_window_height{};  //Save calculated width and height
    CSupportedRenderEnums m_supported_render_enums{};
    DefaultCLazyConstructableWithInitializer<
        CTaskBarDlgDrawCommonWindowSupport,
        CTaskBarDlgDrawCommonSupport&>
        m_taskbar_draw_common_window_support{[]() -> std::tuple<CTaskBarDlgDrawCommonSupport&>
                                             { return {theApp.m_d2d_taskbar_draw_common_support.Get()}; }}; //Provides D2D1 drawing support
    DefaultCLazyConstructableWithInitializer<
        CD2D1DeviceContextWindowSupport,
        CTaskBarDlgDrawCommonSupport&>
        m_d2d1_device_context_support{[]() -> std::tuple<CTaskBarDlgDrawCommonSupport&>
                                      { return {theApp.m_d2d_taskbar_draw_common_support.Get()}; }};

    //The width of various parts of the taskbar
    struct ItemWidth
    {
        int label_width{};      //Label section width
        int value_width{};      //Numeric part width

        int TotalWidth() const  //total width
        {
            return label_width + value_width;
        }

        int MaxWidth() const	//maximum width
        {
            return max(label_width, value_width);
        }
    };

    struct ItemWidthInfo : public CommonDisplayItem
    {
        ItemWidth item_width;
        bool is_double_line{};  //Whether the project occupies two rows

        ItemWidthInfo()
        {}

        ItemWidthInfo(const CommonDisplayItem& item)
            : CommonDisplayItem(item)
        {}
    };

    std::vector<ItemWidthInfo> m_item_widths;   //The width of each portion of the taskbar window actually displayed
    std::map<CommonDisplayItem, CRect> m_item_rects;    //Rectangular area for each part of the taskbar window
    CommonDisplayItem m_clicked_item;           //Taskbar item clicked by mouse

    std::map<CommonDisplayItem, std::list<int>> m_map_history_data;  //A linked list that saves the historical data of each data. The linked list is saved in chronological order. The closer the data is to the head, the more recent it is.
    std::map<CommonDisplayItem, int> m_history_data_count;            //Count the number of times added to the historical data linked list

    bool m_connot_insert_to_task_bar{ false };	//True if the window cannot be embedded in the taskbar
    bool m_taskbar_on_top_or_bottom{ true };		//true if the taskbar is at the top or bottom of the screen
    int m_error_code{};
    bool m_menu_popuped{ false };               //Indicates whether a menu is currently pop-up
    bool m_is_secondary_display{ false };       //Whether to display in the secondary monitor
    bool m_is_width_changed{ false };

    UINT m_taskbar_dpi{};//TaskBarDlg's own dedicated dpi

    CFont m_font;			//font

    CDC* m_pDC{};		//DC of the window, used to calculate the width of the window

    //Find the handle to the taskbar
    //is_scendary_display: Whether the found taskbar is the secondary monitor's taskbar
    HWND FindTaskbarHandle(bool& is_scendary_display);
    CString GetMouseTipsInfo();		//Get mouse tips

    void AddHisToList(CommonDisplayItem item_type, int current_usage_percent);		//Add the current utilization value to the linked list

    int CalculateNetspeedPercent(unsigned __int64 net_speed);     //Calculate the network speed as a percentage of the maximum value of the network speed occupancy graph

    //Determine which display item area a point is in and save it to m_clicked_item. If false is returned, the point is not within the area of any project, otherwise true is returned
    bool CheckClickedItem(CPoint point);

    //Draw a display item in the taskbar window
    //  drawer: object of drawing class
    //  type: type of project
    //  rect: draw a rectangular area
    //  label_width: width of label area
    //  vertical: If true, labels and values are displayed up and down
    void DrawDisplayItem(IDrawCommon& drawer, DisplayItem type, CRect rect, int label_width, bool vertical = false);

    //Draw a plug-in item in the taskbar window
   //  drawer: object of drawing class
   //  item: pointer to the item displayed by the plug-in
   //  rect: draw a rectangular area
   //  label_width: width of label area
   //  vertical: If true, labels and values are displayed up and down
    void DrawPluginItem(IDrawCommon& drawer, IPluginItem* item, CRect rect, int label_width, bool vertical = false);

    void MoveWindow(CRect rect);

public:
    static void DisableRenderFeatureIfNecessary(CSupportedRenderEnums& ref_supported_render_enums);
    void SetTextFont();
    void ApplySettings();
    void CalculateWindowSize();		//Calculate the size of each part of the window, and the width of each part. The window size is saved in m_window_width and m_window_height, the width of each part is saved in m_item_widths, and the rectangular area of each part is saved in m_item_rects

    void SetToolTipsTopMost();			//Set mouse tip to stay on top
    void UpdateToolTips();

    bool GetCannotInsertToTaskBar() const { return m_connot_insert_to_task_bar; }
    int GetErrorCode() const { return m_error_code; }
    bool IsTasksbarOnTopOrBottom() const { return m_taskbar_on_top_or_bottom; }
    bool IsBackgroundLight() const;

    static bool IsItemShow(DisplayItem item);
    static bool IsShowCpuMemory();
    static bool IsShowNetSpeed();

    CommonDisplayItem GetClickedItem() const { return m_clicked_item; }

    //Whether to allow the option "Move taskbar windows closer to icons instead of to the sides of the taskbar"
    //taskbar_wnd_on_left: Whether the taskbar window is on the left side of the taskbar
    static bool IsTaskbarCloseToIconEnable(bool taskbar_wnd_on_left);

    DECLARE_MESSAGE_MAP()

public:
    virtual BOOL OnInitDialog();
    virtual void OnCancel();
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    //afx_msg void OnSetBackgroundColor();
    //afx_msg void OnSetTextColor();
    afx_msg void OnInitMenu(CMenu* pMenu);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //afx_msg void OnSetFont();
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg void OnClose();
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
protected:
    afx_msg LRESULT OnExitmenuloop(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnTabletQuerysystemgesturestatus(WPARAM wParam, LPARAM lParam);
public:
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
};
