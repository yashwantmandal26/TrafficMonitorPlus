// TaskBarDlg.cpp: implementation file
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "TaskBarDlg.h"
#include "afxdialogex.h"
#include "TrafficMonitorDlg.h"
#include "WindowsSettingHelper.h"
#include "WIC.h"
#include "Nullable.hpp"
#include "DrawCommonFactory.h"
#include "WindowsWebExperienceDetector.h"
#include "TaskbarHelper.h"

#ifdef DEBUG
// DX debug information capture
#include "dxgi1_3.h"
#include "DXProgrammableCapture.h"
#endif

// CTaskBarDlg dialog box

IMPLEMENT_DYNAMIC(CTaskBarDlg, CDialogEx)

CTaskBarDlg::CTaskBarDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(IDD_TASK_BAR_DIALOG, pParent)
{
}

CTaskBarDlg::~CTaskBarDlg()
{
    for (auto iter = m_map_history_data.begin(); iter != m_map_history_data.end(); ++iter)
    {
        iter->second.clear();
    }
}

void CTaskBarDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTaskBarDlg, CDialogEx)
    ON_WM_RBUTTONUP()
    ON_WM_INITMENU()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_TIMER()
    ON_WM_PAINT()
    ON_WM_CLOSE()
    ON_WM_LBUTTONUP()
    ON_MESSAGE(WM_EXITMENULOOP, &CTaskBarDlg::OnExitmenuloop)
    ON_MESSAGE(WM_TABLET_QUERYSYSTEMGESTURESTATUS, &CTaskBarDlg::OnTabletQuerysystemgesturestatus)
    ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

// CTaskBarDlg message handler

void CTaskBarDlg::ShowInfo(CDC* pDC)
{
    HWND current_hwnd = this->GetSafeHwnd();
    if (current_hwnd == NULL || !IsWindow(this->GetSafeHwnd()))
        return;
    if (m_rect.IsRectEmpty() || m_rect.IsRectNull())
        return;
    if (m_supported_render_enums.IsD2D1Enabled())
    {
        auto last_update_layered_window_error =
            theApp.m_taskbar_data.update_layered_window_error_code;
        if (last_update_layered_window_error)
        {
            m_supported_render_enums.DisableD2D1();
            if (!m_supported_render_enums.IsD2D1WithDCompositionEnabled())
            {
                m_supported_render_enums.EnableDefaultOnly();
                LogWin32ApiErrorMessage(last_update_layered_window_error);
                CString error_info{};
                error_info.Format(
                    _T("Call UpdateLayeredWindowIndirect failed. Use GDI render instead. Error code = %ld."),
                    last_update_layered_window_error);
                CCommon::WriteLog(error_info, theApp.m_log_path.c_str());
                // Disable D2D
                theApp.m_taskbar_data.disable_d2d = true;
                // Show error message
                ::MessageBox(NULL, CCommon::LoadText(IDS_UPDATE_TASKBARDLG_FAILED_TIP), NULL, MB_OK | MB_ICONWARNING);
            }
        }
    }
    auto render_type = m_supported_render_enums.GetAutoFitEnum();
    CRect draw_rect{m_rect}; // Rectangular area for drawing
    draw_rect.MoveToXY(0, 0);

#ifdef DEBUG
    Microsoft::WRL::ComPtr<IDXGraphicsAnalysis> p_dxgi_analysis{};
#endif
    // Initialize DrawBuffer and DrawCommon stack memory
    AllInvolvedDrawCommonObjectsStorage all_involved_draw_common_objects{};
    IDrawCommon* draw_common_interface{nullptr};
    std::tie(std::ignore, draw_common_interface) =
        GetInterfaceFromAllInvolvedDrawCommonObjects(
            all_involved_draw_common_objects,
            render_type,
            {{DrawCommonHelper::RenderType::DEFAULT,
              [&](IDrawBuffer* p_draw_buffer_interface, IDrawCommon* p_draw_common_interface)
              {
                  auto p_draw_buffer = static_cast<CDrawDoubleBuffer*>(p_draw_buffer_interface);
                  auto p_draw_common = static_cast<CDrawCommon*>(p_draw_common_interface);

                  // Must be constructed before the drawing object
                  EmplaceAt(p_draw_buffer, pDC, draw_rect);
                  // Construct the drawing object here
                  EmplaceAt(p_draw_common);
                  p_draw_common->Create(p_draw_buffer->GetMemDC(), nullptr);
                  p_draw_common->FillRect(draw_rect, theApp.m_taskbar_data.back_color); // fill background color
                  p_draw_common->SetFont(&m_font);
                  p_draw_common->SetBackColor(theApp.m_taskbar_data.back_color);
              }},
             {DrawCommonHelper::RenderType::D2D1_WITH_DCOMPOSITION,
              [&](IDrawBuffer* p_draw_buffer_interface, IDrawCommon* p_draw_common_interface)
              {
                  auto p_draw_buffer = static_cast<CTaskBarDlgDrawBufferUseDComposition*>(p_draw_buffer_interface);
                  auto p_draw_common = static_cast<CTaskBarDlgDrawCommon*>(p_draw_common_interface);

                  auto& ref_d2d1_app_support = theApp.m_d2d_taskbar_draw_common_support.Get();
                  m_d2d1_device_context_support.Get().SetWorkingDevice(
                      ref_d2d1_app_support.GetDCompositionDevice(),
                      ref_d2d1_app_support.GetD3D10Device1(),
                      m_hWnd);
                  // Contrary to the above, DrawCommon is constructed first and then Buffer is constructed.
                  // Construct the drawing object here
                  EmplaceAt(p_draw_common);
                  D2D1_SIZE_U d2d_size;
                  d2d_size.width = draw_rect.Width();
                  d2d_size.height = draw_rect.Height();
                  p_draw_common->Create(
                      this->m_taskbar_draw_common_window_support.Get(),
                      this->m_d2d1_device_context_support.Get(),
                      d2d_size);
                  // This renderer is enabled only when transparent and UpdateLayeredWindowIndirect fails. The default initialization is all black, alpha=1.
                  p_draw_common->FillRect(draw_rect, 0x00000000, 1);
                  p_draw_common->SetFont(&m_font);
                  p_draw_common->SetBackColor(theApp.m_taskbar_data.back_color);
                  // Construct buffer
                  EmplaceAt(p_draw_buffer, this->m_d2d1_device_context_support.Get());
              }},
             {DrawCommonHelper::RenderType::D2D1,
              [&](IDrawBuffer* p_draw_buffer_interface, IDrawCommon* p_draw_common_interface)
              {
                  auto p_draw_buffer = static_cast<CTaskBarDlgDrawBuffer*>(p_draw_buffer_interface);
                  auto p_draw_common = static_cast<CTaskBarDlgDrawCommon*>(p_draw_common_interface);

                  auto& ref_d2d1_app_support = theApp.m_d2d_taskbar_draw_common_support.Get();
                  m_d2d1_device_context_support.Get().SetWorkingDevice(ref_d2d1_app_support.GetD3D10Device1());
                  // Contrary to the above, DrawCommon is constructed first and then Buffer is constructed.
                  // Construct the drawing object here
                  EmplaceAt(p_draw_common);
                  D2D1_SIZE_U d2d_size;
                  d2d_size.width = draw_rect.Width();
                  d2d_size.height = draw_rect.Height();
                  p_draw_common->Create(
                      this->m_taskbar_draw_common_window_support.Get(),
                      this->m_d2d1_device_context_support.Get(),
                      d2d_size);
                  // Enable this renderer only when transparent, initialized to full black by default, alpha=1
                  p_draw_common->FillRect(draw_rect, 0x00000000, 1);
                  p_draw_common->SetFont(&m_font);
                  p_draw_common->SetBackColor(theApp.m_taskbar_data.back_color);
                  // Construct buffer
                  CSize draw_size{draw_rect.Width(), draw_rect.Height()};
                  EmplaceAt(p_draw_buffer,
                            this->m_taskbar_draw_common_window_support.Get(),
                            this->m_d2d1_device_context_support.Get(),
                            draw_size,
                            current_hwnd);

#ifdef DEBUG
                  DXGIGetDebugInterface1(0, IID_PPV_ARGS(&p_dxgi_analysis));
                  if (p_dxgi_analysis)
                  {
                      p_dxgi_analysis->BeginCapture();
                  }
#endif
              }}});
    IDrawCommon& draw = *draw_common_interface;
    // Draw taskbar items
    for (auto iter = m_item_widths.begin(); iter != m_item_widths.end(); ++iter)
    {
        CRect item_rect = m_item_rects[*iter];
        if (!item_rect.IsRectEmpty())
        {
            if (iter->IsPlugin())
                DrawPluginItem(draw, iter->PluginItem(), item_rect, iter->item_width.label_width, iter->is_double_line);
            else
                DrawDisplayItem(draw, iter->ItemType(), item_rect, iter->item_width.label_width, iter->is_double_line);
        }
    }

#ifdef DEBUG
    if (p_dxgi_analysis)
    {
        p_dxgi_analysis->EndCapture();
    }
#endif
}

// 4-Stage Color Definitions: Green -> Yellow -> Orange -> Red (in increasing order)
const COLORREF COLOR_STAGE_GREEN  = RGB(52, 211, 153);  // Soft Mint Emerald (#34D399)
const COLORREF COLOR_STAGE_YELLOW = RGB(251, 191, 36);  // Golden Amber (#FBBF24)
const COLORREF COLOR_STAGE_ORANGE = RGB(251, 146, 60);  // Vivid Coral (#FB923C)
const COLORREF COLOR_STAGE_RED    = RGB(244, 63, 94);   // Electric Rose Red (#F43F5E)
const COLORREF COLOR_GPU_INDICATOR_DOT = RGB(0, 191, 255); // Vibrant Sky Blue (#00BFFF)

// Dynamic color calculation for hardware usage with specific values:
// < 50%       : Green
// 50% - 69%   : Yellow
// 70% - 84%   : Orange
// >= 85%      : Red
static COLORREF GetDynamicUsageColor(int usage)
{
    if (usage < 50)
        return COLOR_STAGE_GREEN;
    else if (usage < 70)
        return COLOR_STAGE_YELLOW;
    else if (usage < 85)
        return COLOR_STAGE_ORANGE;
    else
        return COLOR_STAGE_RED;
}

// Dynamic color calculation for hardware temperature with specific values:
// < 55 deg C       : Green
// 55 - 69 deg C    : Yellow
// 70 - 79 deg C    : Orange
// >= 80 deg C      : Red
static COLORREF GetDynamicTempColor(int temp)
{
    if (temp <= 0)
        return RGB(255, 255, 255); // White if no valid sensor data
    if (temp < 55)
        return COLOR_STAGE_GREEN;
    else if (temp < 70)
        return COLOR_STAGE_YELLOW;
    else if (temp < 80)
        return COLOR_STAGE_ORANGE;
    else
        return COLOR_STAGE_RED;
}

void CTaskBarDlg::DrawDisplayItem(IDrawCommon& drawer, DisplayItem type, CRect rect, int label_width, bool vertical)
{
    //Set the text color to be drawn
    COLORREF label_color{};
    COLORREF text_color{};
    if (theApp.m_taskbar_data.specify_each_item_color)
    {
        label_color = theApp.m_taskbar_data.text_colors[type].label;
        text_color = theApp.m_taskbar_data.text_colors[type].value;
    }
    else if (!theApp.m_taskbar_data.text_colors.empty())
    {
        label_color = theApp.m_taskbar_data.text_colors.begin()->second.label;
        text_color = theApp.m_taskbar_data.text_colors.begin()->second.label;
    }

    // Dynamic color based on usage and temperature (Green -> Yellow -> Red)
    if (theApp.m_taskbar_data.dynamic_color)
    {
        if (type == TDI_CPU)
        {
            label_color = RGB(255, 255, 255); // Keep label ("CPU:") in white
            text_color = GetDynamicUsageColor(theApp.m_cpu_usage); // Only the value (e.g. "100%") changes color
        }
        else if (type == TDI_GPU_USAGE)
        {
            label_color = RGB(255, 255, 255); // Keep label ("GPU:") in white
            text_color = GetDynamicUsageColor(theApp.m_gpu_usage); // Only the value changes color
        }
        else if (type == TDI_MEMORY)
        {
            label_color = RGB(255, 255, 255);
            text_color = GetDynamicUsageColor(theApp.m_memory_usage);
        }
        else if (type == TDI_CPU_TEMP)
        {
            label_color = RGB(255, 255, 255);
            text_color = GetDynamicTempColor(theApp.m_cpu_temperature);
        }
        else if (type == TDI_GPU_TEMP)
        {
            label_color = RGB(255, 255, 255);
            text_color = GetDynamicTempColor(theApp.m_gpu_temperature);
        }
        else if (type == TDI_HDD_TEMP)
        {
            label_color = RGB(255, 255, 255);
            text_color = GetDynamicTempColor(theApp.m_hdd_temperature);
        }
        else if (type == TDI_MAIN_BOARD_TEMP)
        {
            label_color = RGB(255, 255, 255);
            text_color = GetDynamicTempColor(theApp.m_main_board_temperature);
        }
    }

    //Set a rectangular area for labels and values
    CRect rect_label{ rect };
    CRect rect_value{ rect };
    if (!vertical)
    {
        rect_label.right = rect_label.left + label_width;
        rect_value.left = rect_label.right;
    }
    else if (label_width != 0)
    {
        rect_label.bottom = rect_label.top + (rect_label.Height() / 2);
        rect_value.top = rect_label.bottom;
    }

    // Draw status bar
    if (type == TDI_CPU || type == TDI_MEMORY || type == TDI_GPU_USAGE || type == TDI_CPU_TEMP
        || type == TDI_GPU_TEMP || type == TDI_HDD_TEMP || type == TDI_MAIN_BOARD_TEMP || type == TDI_HDD_USAGE
        || type == TDI_UP || type == TDI_DOWN || type == TDI_TOTAL_SPEED/* ||type==TDI_CPU_FREQ*/)
    {
        int figure_value{};
        switch (type)
        {
        case TDI_CPU:
            figure_value = theApp.m_cpu_usage;
            break;
        case TDI_MEMORY:
            figure_value = theApp.m_memory_usage;
            break;
        case TDI_GPU_USAGE:
            figure_value = theApp.m_gpu_usage;
            break;
        case TDI_CPU_TEMP:
            figure_value = theApp.m_cpu_temperature;
            break;
        case TDI_GPU_TEMP:
            figure_value = theApp.m_gpu_temperature;
            break;
        case TDI_HDD_TEMP:
            figure_value = theApp.m_hdd_temperature;
            break;
        case TDI_MAIN_BOARD_TEMP:
            figure_value = theApp.m_main_board_temperature;
            break;
        case TDI_HDD_USAGE:
            figure_value = theApp.m_hdd_usage;
            break;
        //case TDI_CPU_FREQ:
        //    figure_value = theApp.m_cpu_freq;
        //    break;
        case TDI_UP:
            figure_value = CalculateNetspeedPercent(theApp.m_out_speed);
            break;
        case TDI_DOWN:
            figure_value = CalculateNetspeedPercent(theApp.m_in_speed);
            break;
        case TDI_TOTAL_SPEED:
            figure_value = CalculateNetspeedPercent(theApp.m_in_speed + theApp.m_out_speed);
            break;
        default:
            break;
        }

        if ((type != TDI_UP && type != TDI_DOWN && type != TDI_TOTAL_SPEED) && theApp.m_taskbar_data.show_status_bar
            || (type == TDI_UP || type == TDI_DOWN || type == TDI_TOTAL_SPEED) && theApp.m_taskbar_data.show_netspeed_figure)
        {
            if (theApp.m_taskbar_data.cm_graph_type)
            {
                AddHisToList(type, figure_value);
                TryDrawGraph(drawer, rect, type);
            }
            else
            {
                TryDrawStatusBar(drawer, rect, figure_value);
            }
        }
    }

    //draw labels
    if (label_width > 0)
    {
        wstring str_label = theApp.m_taskbar_data.disp_str.GetConst(type);
        CRect draw_label_rect = rect_label;

        if (type == TDI_CPU || type == TDI_GPU_USAGE)
        {
            bool is_dgpu = theApp.IsDgpuActive();
            bool show_dot = (type == TDI_GPU_USAGE && is_dgpu) || (type == TDI_CPU && !is_dgpu);

            int dot_width = 0;
            if (m_pDC != nullptr)
            {
                CFont* pOldFont = m_pDC->SelectObject(&m_font);
                dot_width = m_pDC->GetTextExtent(_T("• ")).cx;
                m_pDC->SelectObject(pOldFont);
            }

            if (show_dot && dot_width > 0)
            {
                CRect rect_dot = draw_label_rect;
                rect_dot.right = rect_dot.left + dot_width;
                drawer.DrawWindowText(rect_dot, _T("•"), COLOR_GPU_INDICATOR_DOT, (vertical ? IDrawCommon::Alignment::CENTER : IDrawCommon::Alignment::LEFT));
            }

            // Offset the label text to the right by dot_width on both CPU and GPU
            // so that "CPU:" and "GPU:" stay 100% vertically aligned
            draw_label_rect.left += dot_width;
        }

        drawer.DrawWindowText(draw_label_rect, str_label.c_str(), label_color, (vertical ? IDrawCommon::Alignment::CENTER : IDrawCommon::Alignment::LEFT));
    }

    //Plot values
    IDrawCommon::Alignment value_alignment{ theApp.m_taskbar_data.value_right_align ? IDrawCommon::Alignment::RIGHT : IDrawCommon::Alignment::LEFT };      //Alignment of values
    if (vertical)
        value_alignment = IDrawCommon::Alignment::CENTER;
    CString str_value = CommonDisplayItem(type).GetItemValueText(false);
    drawer.DrawWindowText(rect_value, str_value, text_color, value_alignment);
}

void CTaskBarDlg::DrawPluginItem(IDrawCommon& drawer, IPluginItem* item, CRect rect, int label_width, bool vertical)
{
    if (item == nullptr)
        return;

    //Draw resource usage graph
    ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(item);
    if (theApp.m_taskbar_data.show_status_bar && plugin != nullptr && plugin->GetAPIVersion() >= 6)
    {
        if (item->IsDrawResourceUsageGraph())
        {
             int figure_value = item->GetResourceUsageGraphValue() * 100;
            //Horizontal scroll chart
            if (theApp.m_taskbar_data.cm_graph_type)
            {
                AddHisToList(item, figure_value);
                TryDrawGraph(drawer, rect, item);
            }
            //bar chart
            else
            {
                TryDrawStatusBar(drawer, rect, figure_value);
            }
        }
    }

    //Set the text color to be drawn
    COLORREF label_text_color{};
    COLORREF value_text_color{};
    if (theApp.m_taskbar_data.specify_each_item_color)
    {
        label_text_color = theApp.m_taskbar_data.text_colors[item].label;
        value_text_color = theApp.m_taskbar_data.text_colors[item].value;
    }
    else if (!theApp.m_taskbar_data.text_colors.empty())
    {
        label_text_color = theApp.m_taskbar_data.text_colors.begin()->second.label;
        value_text_color = theApp.m_taskbar_data.text_colors.begin()->second.label;
    }

    if (plugin != nullptr && plugin->GetAPIVersion() >= 2)
    {
        plugin->OnExtenedInfo(ITMPlugin::EI_DRAW_TASKBAR_WND, L"1");
    }
    if (item->IsCustomDraw())
    {
        //Determine dark or light mode based on the brightness of the background color
        const COLORREF& bk{ theApp.m_taskbar_data.back_color };
        int background_brightness{ (GetRValue(bk) + GetGValue(bk) + GetBValue(bk)) / 3 };
        //Self-drawn by plug-in
        if (plugin != nullptr && plugin->GetAPIVersion() >= 2)
        {
            plugin->OnExtenedInfo(ITMPlugin::EI_LABEL_TEXT_COLOR, std::to_wstring(label_text_color).c_str());
            plugin->OnExtenedInfo(ITMPlugin::EI_VALUE_TEXT_COLOR, std::to_wstring(value_text_color).c_str());
        }
        drawer.SetTextColor(value_text_color);
        //needrtti
        if (typeid(drawer) == typeid(CDrawCommon))
        {
            auto* p_dc = static_cast<CDrawCommon&>(drawer).GetDC();
            item->DrawItem(p_dc->GetSafeHdc(), rect.left, rect.top, rect.Width(), rect.Height(), background_brightness < 128);
            p_dc->SelectObject(&m_font);
        }
        else if (typeid(drawer) == typeid(CTaskBarDlgDrawCommon))
        {
            auto& ref_d2d1_drawer = static_cast<CTaskBarDlgDrawCommon&>(drawer);
            ref_d2d1_drawer.ExecuteGdiOperation(rect,
                [item, rect, background_brightness](HDC gdi_dc)
                { item->DrawItem(gdi_dc,
                    rect.left,
                    rect.top,
                    rect.Width(),
                    rect.Height(),
                    background_brightness < 128); });
        }
        if (plugin != nullptr && plugin->GetAPIVersion() >= 8)
            item->DrawItemEx(&drawer, rect.left, rect.top, rect.Width(), rect.Height(), background_brightness < 128);
    }
    else
    {
        CRect rect_label, rect_value;
        rect_label = rect_value = rect;
        if (label_width > 0)
        {
            if (!vertical)
            {
                rect_label = rect_value = rect;
                rect_label.right = rect_label.left + label_width;
                rect_value.left = rect_label.right;
            }
            else
            {
                rect_label.bottom = rect_label.top + rect.Height() / 2;
                rect_value.top = rect_label.bottom;
            }
        }
        //draw labels
        CString lable_text = theApp.m_taskbar_data.disp_str.GetConst(item).c_str();
        drawer.DrawWindowText(rect_label, lable_text, label_text_color, (vertical ? IDrawCommon::Alignment::CENTER : IDrawCommon::Alignment::LEFT));
        //draw values
        IDrawCommon::Alignment value_alignment{ theApp.m_taskbar_data.value_right_align ? IDrawCommon::Alignment::RIGHT : IDrawCommon::Alignment::LEFT };      //Alignment of values
        if (vertical)
            value_alignment = IDrawCommon::Alignment::CENTER;
        drawer.DrawWindowText(rect_value, item->GetItemValueText(), value_text_color, value_alignment);
    }
}

void CTaskBarDlg::MoveWindow(CRect rect)
{
    if (IsWindow(GetSafeHwnd()))
    {
        ::MoveWindow(GetSafeHwnd(), rect.left, rect.top, rect.Width(), rect.Height(), TRUE);
    }
}

void CTaskBarDlg::DisableRenderFeatureIfNecessary(CSupportedRenderEnums& ref_supported_render_enums)
{
    bool is_transparent = theApp.m_taskbar_data.IsTaskbarTransparent();
    // Disable D2D if UpdateLayeredWindowIndirect fails
    if (theApp.m_taskbar_data.update_layered_window_error_code)
    {
        ref_supported_render_enums.DisableD2D1();
    }
    // If the conditions are not met, Default (MFC) is enabled.
    if (!is_transparent || theApp.m_taskbar_data.auto_set_background_color || theApp.m_taskbar_data.disable_d2d)
    {
        ref_supported_render_enums.EnableDefaultOnly();
    }
}

void CTaskBarDlg::TryDrawStatusBar(IDrawCommon& drawer, const CRect& rect_bar, int usage_percent)
{
    //Limit range
    if (usage_percent > 100)
        usage_percent = 100;
    if (usage_percent < 0)
        usage_percent = 0;
    COLORREF graph_color = theApp.m_taskbar_data.GetUsageGraphColor();
    CSize fill_size = CSize(rect_bar.Width() * usage_percent / 100, rect_bar.Height());
    CRect rect_fill(rect_bar.TopLeft(), fill_size);
    if (theApp.m_taskbar_data.show_graph_dashed_box)
        drawer.DrawRectOutLine(rect_bar, graph_color, 1, true);
    drawer.FillRect(rect_fill, graph_color);
}

bool CTaskBarDlg::AdjustWindowPos(bool force_adjust)
{
    if (this->GetSafeHwnd() == NULL || !IsWindow(this->GetSafeHwnd()))
        return false;

    if (m_is_width_changed)
        force_adjust = true;

    if (force_adjust)
        ResetTaskbarPos();

    ::GetWindowRect(m_hTaskbar, m_rcTaskbar);   //Get the rectangular area of the taskbar

    static bool last_taskbar_on_top_or_bottom;
    CheckTaskbarOnTopOrBottom();
    if (force_adjust || m_taskbar_on_top_or_bottom != last_taskbar_on_top_or_bottom)
    {
        CalculateWindowSize();
        last_taskbar_on_top_or_bottom = m_taskbar_on_top_or_bottom;
        force_adjust = true;
    }

    AdjustTaskbarWndPos(force_adjust);

    //If the window is not successfully embedded in the taskbar and the window is moved to an absolute position based on the upper left corner of the screen, correct the position of the window
    if (m_connot_insert_to_task_bar)
    {
        CRect rc_parent;
        ::GetWindowRect(GetParentHwnd(), rc_parent);
        CRect rect{ m_rect };
        rect.MoveToXY(rect.left + rc_parent.left, rect.top + rc_parent.top);
        this->MoveWindow(rect);

        if (::GetForegroundWindow() == m_hTaskbar)   //When the window cannot be embedded in the taskbar, if the focus is set on the taskbar, keep the window on top
        {
            SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); //Set top
        }
    }

    m_is_width_changed = false;     //Reset flag after adjusting window position
    return true;
}

void CTaskBarDlg::ApplyWindowTransparentColor()
{
#ifndef COMPILE_FOR_WINXP
    if (theApp.IsWindows11Taskbar())      //Do not use pure black as the background color in Windows 11 to solve the problem that the right-click menu cannot pop up in dark mode.
    {
        if (theApp.m_taskbar_data.transparent_color == 0 && theApp.m_taskbar_data.back_color == 0)
        {
            theApp.m_taskbar_data.transparent_color = 1;
            theApp.m_taskbar_data.back_color = 1;
        }
    }
    if ((theApp.m_taskbar_data.transparent_color != 0) && theApp.m_taksbar_transparent_color_enable)
    {
        auto render_type = m_supported_render_enums.GetAutoFitEnum();
        switch (render_type)
        {
            using namespace DrawCommonHelper;
        case RenderType::D2D1_WITH_DCOMPOSITION:
            // D2D drawing and blending using DComposition without setting any properties
            break;
        case RenderType::D2D1:
            // D2D drawing uses alpha blending directly and only sets WS_EX_LAYERED
            SetWindowLong(m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
            break;
        case RenderType::DEFAULT:
            // GDI drawing uses chroma keying
            // Start D2D renderer only when transparent and not using automatically determined background color
            SetWindowLong(m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
            SetLayeredWindowAttributes(theApp.m_taskbar_data.transparent_color, 0, LWA_COLORKEY);
        }
    }
    else
    {
        SetWindowLong(m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
    }
#endif // !COMPILE_FOR_WINXP
}

//bool CTaskBarDlg::IsTaskbarChanged()
//{
//    bool is_scendary_display;
//    static HWND last_taskbar_handle{};
//    HWND taskbar_handle = FindTaskbarHandle(is_scendary_display);
//    bool changed{ false };
//    if (last_taskbar_handle != taskbar_handle)
//    {
//        changed = true;
//        last_taskbar_handle = taskbar_handle;
//    }
//    return changed;
//}

void CTaskBarDlg::WidthChanged()
{
    m_is_width_changed = true;
}

const CRect& CTaskBarDlg::GetRectForDpiCheck() const
{
    return m_rcTaskbar;
}

UINT CTaskBarDlg::GetDPI() const
{
    return m_taskbar_dpi;
}

void CTaskBarDlg::SetDPI(UINT dpi)
{
    m_taskbar_dpi = dpi;
}

UINT CTaskBarDlg::DPI(UINT pixel) const
{
    return m_taskbar_dpi * pixel / 96;
}

int CTaskBarDlg::DPI(int pixel) const
{
    return static_cast<int>(m_taskbar_dpi) * pixel / 96;
}

LONG CTaskBarDlg::DPI(LONG pixel) const
{
    return static_cast<LONG>(m_taskbar_dpi) * pixel / 96;
}

void CTaskBarDlg::DPI(CRect& rect) const
{
    rect.left = DPI(rect.left);
    rect.right = DPI(rect.right);
    rect.top = DPI(rect.top);
    rect.bottom = DPI(rect.bottom);
}

CTaskBarDlg::ClassCheckWindowMonitorDPIAndHandle CTaskBarDlg::CheckWindowMonitorDPIAndHandle{};

UINT CTaskBarDlg::ClassCheckWindowMonitorDPIAndHandle::buffered_dpi_x{0};
UINT CTaskBarDlg::ClassCheckWindowMonitorDPIAndHandle::buffered_dpi_y{0};
UINT CTaskBarDlg::ClassCheckWindowMonitorDPIAndHandle::dpi_x{0};
UINT CTaskBarDlg::ClassCheckWindowMonitorDPIAndHandle::dpi_y{0};

HWND CTaskBarDlg::FindTaskbarHandle(bool& is_scendary_display)
{
    is_scendary_display = false;
    HWND hTaskbar = nullptr;
    //displayed on secondary monitor
    if (theApp.m_taskbar_data.show_taskbar_wnd_in_secondary_display && CWindowsSettingHelper::IsTaskbarShowingInAllDisplays())
    {
        //Get the taskbar of all secondary monitors
        std::vector<HWND> secondary_taskbars;
        CTaskbarHelper::GetAllSecondaryDisplayTaskbar(secondary_taskbars);
        if (!secondary_taskbars.empty())
        {
            int index = theApp.m_taskbar_data.secondary_display_index;
            if (index < 0)
                index = 0;
            if (index >= static_cast<int>(secondary_taskbars.size()))
                index = static_cast<int>(secondary_taskbars.size()) - 1;
            hTaskbar = secondary_taskbars[index];
            if (hTaskbar != nullptr)
                is_scendary_display = true;
        }
    }
    if (hTaskbar == nullptr)
        hTaskbar = ::FindWindow(_T("Shell_TrayWnd"), NULL);

    return hTaskbar;
}

CString CTaskBarDlg::GetMouseTipsInfo()
{
    CString tip_info;
    CString temp;
    tip_info = _T("\x26A1 System Hardware Monitor\r\n-------------------------\r\n");
    temp.Format(_T("• Mode: %s\r\n"), theApp.IsDgpuActive() ? _T("dGPU Active") : _T("iGPU Active"));
    tip_info += temp;
    temp.Format(_T("• CPU: %d%% (%d\x00B0C)\r\n"), theApp.m_cpu_usage, theApp.m_cpu_temperature);
    tip_info += temp;
    if (theApp.m_gpu_usage >= 0)
    {
        temp.Format(_T("• GPU: %d%% (%d\x00B0C)\r\n"), theApp.m_gpu_usage, theApp.m_gpu_temperature);
        tip_info += temp;
    }
    temp.Format(_T("• RAM: %s / %s (%d%%)\r\n"),
        CCommon::KBytesToString(theApp.m_used_memory),
        CCommon::KBytesToString(theApp.m_total_memory),
        theApp.m_memory_usage);
    tip_info += temp;
    temp.Format(_T("• Net: \x2191 %s/s  \x2193 %s/s"),
        CCommon::DataSizeToString(theApp.m_out_speed, theApp.m_taskbar_data),
        CCommon::DataSizeToString(theApp.m_in_speed, theApp.m_taskbar_data));
    tip_info += temp;
    return tip_info;
}

void CTaskBarDlg::SetTextFont()
{
    //If m_font is already associated with a font resource object, release it
    if (m_font.m_hObject)
    {
        m_font.DeleteObject();
    }
    //Create new font
    theApp.m_taskbar_data.font.Create(m_font, GetDPI());
}

void CTaskBarDlg::ApplySettings()
{
    SetTextFont();
    CalculateWindowSize();
}

void CTaskBarDlg::CalculateWindowSize()
{
    bool horizontal_arrange = theApp.m_taskbar_data.horizontal_arrange && m_taskbar_on_top_or_bottom;
    if (theApp.m_taskbar_data.display_item.IsEmpty() && theApp.m_taskbar_data.plugin_display_item.data().empty())
        theApp.m_taskbar_data.display_item.Add(TDI_UP);        //Show at least one

    m_item_widths.clear();
    //Display item width
    std::map<CommonDisplayItem, ItemWidth> item_widths;

    m_pDC->SelectObject(&m_font);
    //Calculate width of labels and values
    //const auto& item_map = theApp.m_taskbar_data.disp_str.GetAllItems();
    for (auto iter = theApp.m_plugins.AllDisplayItemsWithPlugins().begin(); iter != theApp.m_plugins.AllDisplayItemsWithPlugins().end(); ++iter)
    {
        if (iter->IsPlugin())
        {
            auto plugin = iter->PluginItem();
            if (plugin != nullptr && theApp.m_taskbar_data.plugin_display_item.Contains(plugin->GetItemId()))
            {
                //label width
                int& label_width{ item_widths[*iter].label_width };
                //Numeric width
                int& value_width{ item_widths[plugin].value_width };
                if (plugin->IsCustomDraw())
                {
                    label_width = 0;
                    value_width = theApp.m_plugins.GetItemWidth(plugin, m_pDC);
                }
                else
                {
                    CString lable_text = theApp.m_taskbar_data.disp_str.GetConst(plugin).c_str();
                    label_width = m_pDC->GetTextExtent(lable_text).cx;
                    value_width = m_pDC->GetTextExtent(plugin->GetItemValueSampleText()).cx;
                }
            }
        }
        else
        {
            //label width
            int dot_extra = 0;
            if (*iter == TDI_CPU || *iter == TDI_GPU_USAGE)
            {
                dot_extra = m_pDC->GetTextExtent(_T("• ")).cx;
            }
            item_widths[*iter].label_width = m_pDC->GetTextExtent(theApp.m_taskbar_data.disp_str.GetConst(*iter).c_str()).cx + dot_extra;
            //Numeric width
            CString sample_str = iter->GetItemValueSampleText(false);
            item_widths[*iter].value_width = m_pDC->GetTextExtent(sample_str).cx;
        }
    }

    auto item_order{ theApp.m_taskbar_data.item_order.GetAllDisplayItemsWithOrder() };
    for (const auto& item : item_order)
    {
        if (theApp.IsTaksbarItemDisplayed(item))
        {
            ItemWidthInfo width_info = item;
            width_info.item_width = item_widths[item];
            m_item_widths.push_back(width_info);
        }
    }

    int item_count = static_cast<int>(m_item_widths.size());

    auto last_window_width{ m_window_width };
    auto last_window_height{ m_window_height };

    //Calculate total window width/height and rectangular area
    if (IsTasksbarOnTopOrBottom())  //When the taskbar is at the top or bottom of the desktop
    {
        if (!horizontal_arrange)
            m_window_height = TASKBAR_WND_HEIGHT;
        else
            m_window_height = TASKBAR_WND_HEIGHT * 2 / 3;

        m_window_width = 0;
        if (theApp.m_taskbar_data.horizontal_arrange)   //When arranged horizontally
        {
            CRect item_rect{};
            int index = 0;
            for (auto iter = m_item_widths.begin(); iter != m_item_widths.end(); ++iter)
            {
                m_window_width += iter->item_width.TotalWidth();
                if (index > 0)
                    item_rect.MoveToX(item_rect.right + DPI(theApp.m_taskbar_data.item_space));
                item_rect.right = item_rect.left + iter->item_width.TotalWidth();
                item_rect.bottom = item_rect.top + m_window_height;
                m_item_rects[*iter] = item_rect;
                index++;
            }
            m_window_width += DPI(theApp.m_taskbar_data.item_space) * item_count;   //Add space between each label
        }
        else        //When not arranged horizontally, arrange every two in groups
        {
            int current_x = DPI(theApp.m_taskbar_data.item_space); // The initial X coordinate is the left gap
            int item_space = DPI(theApp.m_taskbar_data.item_space);

            //Calculate the vertical position of the upper and lower regions when grouped in two
            int item_height = m_window_height / 2;
            int y_pos_up = -DPI(theApp.m_taskbar_data.vertical_margin) / 2;
            int y_pos_down = y_pos_up + item_height + DPI(theApp.m_taskbar_data.vertical_margin);

            int width0 = 0;
            int max_width0 = 0;
            bool has_first_item = false;
            auto first_iter = m_item_widths.end(); // Iterator used to save the first item of the current column so that its width can be corrected later

            for (auto iter = m_item_widths.begin(); iter != m_item_widths.end(); ++iter)
            {
                if (iter->IsDoubleLineExclusive())
                {
                    // If there was an unpaired single line item before it, it is now forced into its own column and should take up two lines
                    if (has_first_item)
                    {
                        int col_width = max_width0; // When occupying two lines, the width uses MaxWidth()
                        m_item_rects[*first_iter].SetRect(current_x, 0, current_x + col_width, m_window_height);
                        first_iter->is_double_line = true;

                        current_x += col_width + item_space;
                        has_first_item = false;
                    }

                    // Process the current exclusive double-line item, separate it into a column, and occupy two lines
                    int col_width = iter->item_width.MaxWidth();
                    m_item_rects[*iter].SetRect(current_x, 0, current_x + col_width, m_window_height);
                    iter->is_double_line = true;

                    current_x += col_width + item_space;
                }
                else
                {
                    iter->is_double_line = false;
                    if (!has_first_item)
                    {
                        // The first item (top half) of the current column, occupying one row
                        width0 = iter->item_width.TotalWidth();
                        max_width0 = iter->item_width.MaxWidth();

                        first_iter = iter;
                        // Temporarily set rect, right bounds will be corrected based on column maximum width when pairing is successful or loop ends
                        m_item_rects[*iter].SetRect(current_x, y_pos_up, current_x + width0, y_pos_up + item_height);

                        has_first_item = true;
                    }
                    else
                    {
                        // The second item (lower half) of the current column, occupying a row and forming a column with the first item
                        int col_width = max(width0, iter->item_width.TotalWidth());

                        // Correct the right margin of the first item to ensure that both items in the same column have the same width
                        m_item_rects[*first_iter].right = current_x + col_width;

                        // Set the rectangular area of the second item
                        m_item_rects[*iter].SetRect(current_x, y_pos_down, current_x + col_width, y_pos_down + item_height);

                        current_x += col_width + item_space;
                        has_first_item = false;
                    }
                }
            }

            // After the loop ends, if there are still unpaired single-row items (for example, the total number of items is an odd number, and the last one is not an exclusive double row), it is columned alone and should occupy two rows.
            if (has_first_item)
            {
                int col_width = max_width0; // When occupying two lines, the width uses MaxWidth()
                m_item_rects[*first_iter].SetRect(current_x, 0, current_x + col_width, m_window_height);
                first_iter->is_double_line = true;

                current_x += col_width + item_space;
            }

            // save total width
            m_window_width = current_x;
        }
    }
    else        //When the taskbar is on both sides of the desktop
    {
        m_window_width = 0;
        m_window_height = 0;
        CRect item_rect;
        for (auto iter = m_item_widths.begin(); iter != m_item_widths.end(); ++iter)
        {
            //The largest width among all labels is the window width
            if (m_window_width < iter->item_width.TotalWidth())
                m_window_width = iter->item_width.TotalWidth();

            int item_height = TASKBAR_WND_HEIGHT / 2;
            if (iter->IsDoubleLineExclusive())
                item_height = TASKBAR_WND_HEIGHT;

            //Calculate rectangular area
            item_rect.top = m_window_height;
            item_rect.bottom = item_rect.top + item_height;
            int item_width = min(m_window_width, m_rcTaskbar.Width() - DPI(2));
            item_rect.left = 0;
            item_rect.right = item_rect.left + item_width;
            m_item_rects[*iter] = item_rect;

            m_window_height += item_height;
            m_window_height += DPI(theApp.m_taskbar_data.item_space);
        }
    }

    // Reposition the taskbar window if the window size changes
    if (last_window_width != m_window_width || last_window_height != m_window_height) {
        WidthChanged();
    }

    m_rect.right = m_rect.left + m_window_width;
    m_rect.bottom = m_rect.top + m_window_height;

}

void CTaskBarDlg::SetToolTipsTopMost()
{
    m_tool_tips.SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
}

void CTaskBarDlg::UpdateToolTips()
{
    if (theApp.m_taskbar_data.show_tool_tip && IsWindow(m_tool_tips.GetSafeHwnd()))
    {
        CString tip_info;
        tip_info = GetMouseTipsInfo();
        m_tool_tips.UpdateTipText(tip_info, this);
    }
}

bool CTaskBarDlg::IsItemShow(DisplayItem item)
{
    return (theApp.m_taskbar_data.display_item.Contains(item));
}

bool CTaskBarDlg::IsShowCpuMemory()
{
    return (theApp.m_taskbar_data.display_item.Contains(TDI_CPU) || theApp.m_taskbar_data.display_item.Contains(TDI_MEMORY));
}

bool CTaskBarDlg::IsShowNetSpeed()
{
    return (theApp.m_taskbar_data.display_item.Contains(TDI_UP) || theApp.m_taskbar_data.display_item.Contains(TDI_DOWN));
}

bool CTaskBarDlg::IsTaskbarCloseToIconEnable(bool taskbar_wnd_on_left)
{
    //In Windows 11, when the taskbar window is displayed on the right, if you check "Taskbar window is closer to the icon instead of closer to the taskbar window", it will overlap with the icon.
    //Because the rectangular area of the "MSTaskSwWClass" window is no longer the area where the taskbar icon minimize button is located
    return theApp.m_win_version.IsWindows11OrLater() &&
        CWindowsSettingHelper::IsTaskbarCenterAlign() && taskbar_wnd_on_left;
}

BOOL CTaskBarDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // TODO: Add additional initialization here
    SetWindowText(TASKBAR_WINDOW_NAME);
    // Detect whether MicrosoftWindows.Client.WebExperience (aka Windows Web Experience Pack) is installed on the system
    theApp.m_taskbar_data.is_windows_web_experience_detected =
        WindowsWebExperienceDetector::IsDetected();
    // Disable necessary rendering options according to the settings of the taskbar window, D2D rendering will only be used if it is transparent and supports D2D rendering
    DisableRenderFeatureIfNecessary(m_supported_render_enums);
    //Set to hide taskbar icons
    ModifyStyleEx(0, WS_EX_TOOLWINDOW);

    m_pDC = GetDC();

    m_hTaskbar = FindTaskbarHandle(m_is_secondary_display); //Find the handle to the taskbar
    ::GetWindowRect(m_hTaskbar, m_rcTaskbar);   //Get the rectangular area of the taskbar

    //Set window transparent color
    ApplyWindowTransparentColor();

    InitTaskbarWnd();
    m_connot_insert_to_task_bar = !(::SetParent(this->m_hWnd, GetParentHwnd())); //Set the program window as a child window of the taskbar

    //Minimize the window area based on the determined taskbar to get the screen and get the DPI of the screen (Windows 8.1 and above)
    if (theApp.m_win_version.IsWindows8Point1OrLater())
    {
        UINT dpi_x, dpi_y;
        if (theApp.DPIFromRect(GetRectForDpiCheck(), &dpi_x, &dpi_y))
            SetDPI(dpi_x);
        else
            SetDPI(theApp.GetDpi());
    }
    else
    {
        SetDPI(theApp.GetDpi());
    }

    //Set font
    SetTextFont();
    m_pDC->SelectObject(&m_font);

    CheckTaskbarOnTopOrBottom();
    CalculateWindowSize();
    m_rect.SetRectEmpty();
    m_rect.bottom = m_window_height;
    m_rect.right = m_rect.left + m_window_width;
    m_error_code = GetLastError();
    AdjustWindowPos(true);

    SetBackgroundColor(theApp.m_taskbar_data.back_color);

    //Initialize mouse tips
    if (IsWindow(GetSafeHwnd()) && m_tool_tips.Create(this, TTS_ALWAYSTIP) && IsWindow(m_tool_tips.GetSafeHwnd()))
    {
        m_tool_tips.SetMaxTipWidth(600);
        m_tool_tips.AddTool(this, _T(""));
        SetToolTipsTopMost();       //Set reminder information to always stay on top
    }

    SetTimer(TASKBAR_TIMER, 1000, NULL);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // Exception: OCX property page should return FALSE
}

void CTaskBarDlg::OnCancel()
{
    // TODO: Add specialized code here and/or call the base class
    //SaveConfig();

    //Close all windows whose parent window is the taskbar window
    for (const auto& item : CBaseDialog::AllUniqueHandels())
    {
        HWND parent = ::GetParent(item.second);
        if (parent == GetSafeHwnd())
        {
            ::SendMessage(item.second, WM_COMMAND, IDCANCEL, 0);
        }
    }

    DestroyWindow();
    ResetTaskbarPos();

    //CDialogEx::OnCancel();
}

void CTaskBarDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
    // TODO: Add message handler code and/or call defaults here
    m_menu_popuped = true;
    m_tool_tips.Pop();
    ITMPlugin* plugin{};
    bool is_plugin_item_clicked = (CheckClickedItem(point) && m_clicked_item.IsPlugin() && m_clicked_item.PluginItem() != nullptr);
    if (is_plugin_item_clicked)
    {
        plugin = theApp.m_plugins.GetPluginByItem(m_clicked_item.PluginItem());
        if (plugin != nullptr && plugin->GetAPIVersion() >= 3)
        {
            if (m_clicked_item.PluginItem()->OnMouseEvent(IPluginItem::MT_RCLICKED, point.x, point.y, (void*)GetSafeHwnd(), IPluginItem::MF_TASKBAR_WND) != 0)
                return;
        }
    }

    CPoint point1;  //Define a position used to determine the cursor position
    GetCursorPos(&point1);  //Get the current cursor position so that the menu can follow the cursor
    CMenu* pMenu = (is_plugin_item_clicked ? theApp.m_taskbar_menu_plugin.GetSubMenu(0) : theApp.m_taskbar_menu.GetSubMenu(0));
    if (pMenu != nullptr)
    {
        if (plugin != nullptr)
        {
            //Change the displayed text of the plug-in menu in the right-click menu to the plug-in name
            const int PLUGIN_ITEM_INDEX = pMenu->GetMenuItemCount() - 1;    //The plug-in item is the last item in the menu
            pMenu->ModifyMenu(PLUGIN_ITEM_INDEX, MF_BYPOSITION, PLUGIN_ITEM_INDEX, plugin->GetInfo(ITMPlugin::TMI_NAME));
            //Get plugin icon
            HICON plugin_icon{};
            if (plugin->GetAPIVersion() >= 5)
                plugin_icon = (HICON)plugin->GetPluginIcon();
            //Set plugin icon
            if (plugin_icon != nullptr)
                CMenuIcon::AddIconToMenuItem(pMenu->GetSafeHmenu(), PLUGIN_ITEM_INDEX, TRUE, plugin_icon);
        }
        //Update plugin submenu
        theApp.UpdatePluginMenu(&theApp.m_taskbar_menu_plugin_sub_menu, plugin, 2);
        //Remove Main Window option
        pMenu->DeleteMenu(ID_SHOW_MAIN_WND, MF_BYCOMMAND);
        //Popup menu
        pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point1.x, point1.y, this);
    }
    CDialogEx::OnRButtonUp(nFlags, point1);
}

void CTaskBarDlg::OnInitMenu(CMenu* pMenu)
{
    CDialogEx::OnInitMenu(pMenu);

    pMenu->DeleteMenu(ID_SHOW_MAIN_WND, MF_BYCOMMAND);

    // Set default menu item (Task Manager by default)
    switch (theApp.m_taskbar_data.double_click_action)
    {
    case DoubleClickAction::CONNECTION_INFO:
        pMenu->SetDefaultItem(ID_NETWORK_INFO);
        break;
    case DoubleClickAction::OPTIONS:
        pMenu->SetDefaultItem(ID_OPTIONS2);
        break;
    case DoubleClickAction::TASK_MANAGER:
    default:
        pMenu->SetDefaultItem(ID_OPEN_TASK_MANAGER);
        break;
    }
}

BOOL CTaskBarDlg::PreTranslateMessage(MSG* pMsg)
{
    // TODO: Add specialized code here and/or call the base class
    //To shield, press Enter and ESC to exit.
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE) return TRUE;
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) return TRUE;

    if (theApp.m_taskbar_data.show_tool_tip && !m_menu_popuped && IsWindow(m_tool_tips.GetSafeHwnd()) && (pMsg->message == WM_LBUTTONDOWN ||
        pMsg->message == WM_LBUTTONUP ||
        pMsg->message == WM_MOUSEMOVE))
    {
        m_tool_tips.RelayEvent(pMsg);
    }

    if (pMsg->message == WM_KEYDOWN)
    {
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x80);
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000);
        bool alt = (GetKeyState(VK_MENU) & 0x8000);
        ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(m_clicked_item.PluginItem());
        if (plugin != nullptr && plugin->GetAPIVersion() >= 4)
        {
            if (m_clicked_item.PluginItem()->OnKeboardEvent(pMsg->wParam, ctrl, shift, alt, (void*)GetSafeHwnd(), IPluginItem::KF_TASKBAR_WND) != 0)
                return TRUE;
        }
    }

    return CDialogEx::PreTranslateMessage(pMsg);
}

void CTaskBarDlg::OnMouseMove(UINT nFlags, CPoint point)
{
    // TODO: Add message handler code and/or call defaults here

    CDialogEx::OnMouseMove(nFlags, point);
}

void CTaskBarDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    // TODO: Add message handler code and/or call defaults here
    if (CheckClickedItem(point) && m_clicked_item.IsPlugin() && m_clicked_item.PluginItem() != nullptr)
    {
        ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(m_clicked_item.PluginItem());
        if (plugin != nullptr && plugin->GetAPIVersion() >= 3)
        {
            if (m_clicked_item.PluginItem()->OnMouseEvent(IPluginItem::MT_DBCLICKED, point.x, point.y, (void*)GetSafeHwnd(), IPluginItem::MF_TASKBAR_WND) != 0)
                return;
        }
    }
    switch (theApp.m_taskbar_data.double_click_action)
    {
    case DoubleClickAction::CONNECTION_INFO:
        SendMessage(WM_COMMAND, ID_NETWORK_INFO);        //After double-clicking, the "Connection Details" dialog box will pop up.
        break;
    case DoubleClickAction::HISTORY_TRAFFIC:
        SendMessage(WM_COMMAND, ID_TRAFFIC_HISTORY);        //After double-clicking, the "Historical Traffic Statistics" dialog box will pop up.
        break;
    case DoubleClickAction::SHOW_MORE_INFO:
        PostMessage(WM_COMMAND, ID_SHOW_CPU_MEMORY2);       //Toggle display of CPU and memory utilization
        break;
    case DoubleClickAction::OPTIONS:
        SendMessage(WM_COMMAND, ID_OPTIONS2);       //After double-clicking, the "Option Settings" dialog box will pop up.
        break;
    case DoubleClickAction::TASK_MANAGER:
        ShellExecuteW(NULL, _T("open"), (theApp.m_system_dir + L"\\Taskmgr.exe").c_str(), NULL, NULL, SW_NORMAL);       //Open task manager
        break;
    case DoubleClickAction::SEPCIFIC_APP:
        ShellExecuteW(NULL, _T("open"), (theApp.m_taskbar_data.double_click_exe).c_str(), NULL, NULL, SW_NORMAL);   //Open the specified program, the default task manager
        break;
    default:
        break;
    }
    //CDialogEx::OnLButtonDblClk(nFlags, point);
}

void CTaskBarDlg::OnTimer(UINT_PTR nIDEvent)
{
    // TODO: Add message handler code and/or call defaults here
    if (nIDEvent == TASKBAR_TIMER)
    {
        if (m_menu_popuped)     //When the right-click menu is displayed, the mouse prompt is not displayed
        {
            m_tool_tips.Pop();
        }
        
        //Recalculate the width of the window every second
        CalculateWindowSize();
    }

    CDialogEx::OnTimer(nIDEvent);
}

BOOL CTaskBarDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    // TODO: Add specialized code here and/or call the base class
    UINT uMsg = LOWORD(wParam);
    if (uMsg == ID_SELECT_ALL_CONNECTION || uMsg == ID_SELETE_CONNECTION
        || (uMsg > ID_SELECT_ALL_CONNECTION && uMsg <= ID_SELETE_CONNECTION_MAX))
    {
        ::SendMessage(theApp.m_pMainWnd->GetSafeHwnd(), WM_COMMAND, wParam, lParam);    //If the "Select Network Connection" submenu item is clicked, forward the message to the main window
        return TRUE;
    }

    //Plug-in project selected in "Show Projects"
    if (uMsg >= ID_SHOW_PLUGIN_ITEM_START && uMsg <= ID_SHOW_PLUGIN_ITEM_MAX)
    {
        IPluginItem* item = theApp.m_plugins.GetItemByIndex(uMsg - ID_SHOW_PLUGIN_ITEM_START);
        if (item != nullptr)
        {
            bool displayed = theApp.m_taskbar_data.plugin_display_item.Contains(item->GetItemId());
            theApp.m_taskbar_data.plugin_display_item.SetStrContained(item->GetItemId(), !displayed);
            ::PostMessage(theApp.m_pMainWnd->GetSafeHwnd(), WM_REOPEN_TASKBAR_WND, 0, 0);
        }
    }
    //Plugin command selected
    if (uMsg >= ID_PLUGIN_COMMAND_START && uMsg <= ID_PLUGIN_COMMAND_MAX)
    {
        int index = uMsg - ID_PLUGIN_COMMAND_START;
        if (m_clicked_item.IsPlugin() && m_clicked_item.PluginItem() != nullptr)
        {
            ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(m_clicked_item.PluginItem());
            if (plugin != nullptr && plugin->GetAPIVersion() >= 5)
            {
                plugin->OnPluginCommand(index, (void*)GetSafeHwnd(), nullptr);
            }
        }
    }

    return CDialogEx::OnCommand(wParam, lParam);
}

void CTaskBarDlg::OnPaint()
{
    CPaintDC dc(this); // device context for painting
                       // TODO: add message handler code here
                       // CDialogEx::OnPaint() is not called for drawing messages

    try
    {
        ShowInfo(&dc);
    }
    catch (CD3D10Exception1& ex)
    {
        DrawCommonHelper::DefaultD2DDrawCommonExceptionHandler{ex}(
            [p_window_support_wrapper = &this->m_taskbar_draw_common_window_support](CHResultException& ex)
            {
                return DrawCommonHelper::HandleIfNeedRecreate(
                    ex,
                    [p_window_support_wrapper]()
                    { p_window_support_wrapper->Get().RequestD3D10Device1Recreate(); });
            });
    }
    catch (CD2D1Exception& ex)
    {
        DrawCommonHelper::DefaultD2DDrawCommonExceptionHandler{ex}(
            [p_device_context_support_wrapper = &this->m_d2d1_device_context_support](CHResultException& ex)
            {
                return DrawCommonHelper::HandleIfD2D1DeviceNeedRecreate(
                    ex,
                    [&]()
                    { p_device_context_support_wrapper->Get().RequestD2D1DeviceRecreate(ex.GetHResult()); });
            });
    }
    catch (CDCompositionException& ex)
    {
        DrawCommonHelper::DefaultD2DDrawCommonExceptionHandler{ex}(
            [p_device_context_support_wrapper = &this->m_d2d1_device_context_support](CHResultException& ex)
            {
                p_device_context_support_wrapper->Get().RequestDCompositionDeviceRecreate(ex.GetHResult());
                return true;
            });
    }
    catch (CHResultException& ex)
    {
        DrawCommonHelper::DefaultD2DDrawCommonExceptionHandler{ex}();
    }
    catch (std::runtime_error& ex)
    {
        auto* log = ex.what();
        CCommon::WriteLog(log, theApp.m_log_path.c_str());
        // Currently, it is the only one that actively throws exceptions, and all exceptions are counted on it.
        DrawCommonHelper::DefaultD2DDrawCommonExceptionHandler::IncreaseErrorCountManually();
        DrawCommonHelper::DefaultD2DDrawCommonExceptionHandler::HandleErrorCountIncrement();
    }

}

void CTaskBarDlg::AddHisToList(CommonDisplayItem item_type, int current_usage_percent)
{
    int& data_count{ m_history_data_count[item_type] };
    std::list<int>& list = m_map_history_data[item_type];
    //Accumulate the numbers to the head of the linked list and calculate the average until the number of added data reaches a multiple of TASKBAR_GRAPH_STEP
    if (data_count % TASKBAR_GRAPH_STEP == 0)
    {
        //Calculate the average of the previously accumulated TASKBAR_GRAPH_STEP data
        if (!list.empty())
            list.front() /= TASKBAR_GRAPH_STEP;
        //Add new data to the end
        list.push_front(current_usage_percent);
    }
    else
    {
        //Numbers are added to the head of the linked list
        list.front() += current_usage_percent;
    }
    size_t graph_max_length = m_item_rects[item_type].Width();
    //Determine whether the maximum length is exceeded. If so, remove the data at the end of the linked list.
    if (list.size() > graph_max_length)
    {
        list.pop_back();
    }
    data_count++;
}

int CTaskBarDlg::CalculateNetspeedPercent(unsigned __int64 net_speed)
{
    int percet = 0;
    unsigned __int64 max_value{ theApp.m_taskbar_data.GetNetspeedFigureMaxValueInBytes() };

    if (net_speed >= max_value)
        percet = 100;
    else if (max_value > 0)
        percet = net_speed * 100 / max_value;
    return percet;
}

bool CTaskBarDlg::CheckClickedItem(CPoint point)
{
    for (const auto& item : m_item_rects)
    {
        if (item.second.PtInRect(point))
        {
            m_clicked_item = item.first;
            return true;
        }
    }
    m_clicked_item = TDI_UP;
    return false;
}

void CTaskBarDlg::TryDrawGraph(IDrawCommon& drawer, const CRect& value_rect, CommonDisplayItem item_type)
{
    std::list<int>& list = m_map_history_data[item_type];
    COLORREF graph_color = theApp.m_taskbar_data.GetUsageGraphColor();
    if (theApp.m_taskbar_data.show_graph_dashed_box)
        drawer.DrawRectOutLine(value_rect, graph_color, 1, true);
    int i{ -1 };
    for (int value : list)
    {
        i++;
        if (i == 0)     //The data at the head of the linked list is not drawn because the average has not been taken during the accumulation.
            continue;
        if (i >= value_rect.Width())
            break;
        //Limit range
        if (value > 100)
            value = 100;
        if (value < 0)
            value = 0;
        //Draw lines from right to left
        CPoint start_point = CPoint(value_rect.right - i, value_rect.bottom);
        int height = value * value_rect.Height() / 100;
        drawer.DrawLine(start_point, height, graph_color);
    }
}

void CTaskBarDlg::OnClose()
{
    // TODO: Add message handler code and/or call defaults here
    ::SendMessage(theApp.m_pMainWnd->GetSafeHwnd(), WM_TASKBAR_WND_CLOSED, 0, 0);

    CDialogEx::OnClose();
}

void CTaskBarDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
    // TODO: Add message handler code and/or call defaults here
    if (CheckClickedItem(point) && m_clicked_item.IsPlugin() && m_clicked_item.PluginItem() != nullptr)
    {
        ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(m_clicked_item.PluginItem());
        if (plugin != nullptr && plugin->GetAPIVersion() >= 3)
        {
            if (m_clicked_item.PluginItem()->OnMouseEvent(IPluginItem::MT_LCLICKED, point.x, point.y, (void*)GetSafeHwnd(), IPluginItem::MF_TASKBAR_WND) != 0)
                return;
        }
    }

    CDialogEx::OnLButtonUp(nFlags, point);
}

afx_msg LRESULT CTaskBarDlg::OnExitmenuloop(WPARAM wParam, LPARAM lParam)
{
    m_menu_popuped = false;
    return 0;
}


afx_msg LRESULT CTaskBarDlg::OnTabletQuerysystemgesturestatus(WPARAM wParam, LPARAM lParam)
{
    return 0;
}


BOOL CTaskBarDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    if (zDelta >= 120 || zDelta <= -120)
    {
        CPoint point = pt;
        ScreenToClient(&point);
        ITMPlugin* plugin{};
        bool is_plugin_item_clicked = (CheckClickedItem(point) && m_clicked_item.IsPlugin() && m_clicked_item.PluginItem() != nullptr);
        if (is_plugin_item_clicked)
        {
            plugin = theApp.m_plugins.GetPluginByItem(m_clicked_item.PluginItem());
            if (plugin != nullptr && plugin->GetAPIVersion() >= 3)
            {
                IPluginItem::MouseEventType type;
                if (zDelta > 0)
                    type = IPluginItem::MT_WHEEL_UP;
                else
                    type = IPluginItem::MT_WHEEL_DOWN;
                if (m_clicked_item.PluginItem()->OnMouseEvent(type, point.x, point.y, (void*)GetSafeHwnd(), IPluginItem::MF_TASKBAR_WND) != 0)
                    return TRUE;
            }
        }
    }

    return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}
