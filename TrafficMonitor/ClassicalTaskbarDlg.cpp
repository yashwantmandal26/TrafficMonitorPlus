#include "stdafx.h"
#include "ClassicalTaskbarDlg.h"

void CClassicalTaskbarDlg::AdjustTaskbarWndPos(bool force_adjust)
{
    ::GetWindowRect(m_hMin, m_rcMin); //Get the area of the minimized window
    ::GetWindowRect(m_hBar, m_rcBar); //Get the area of the secondary container

    if (m_taskbar_on_top_or_bottom)     //When the taskbar is at the top or bottom of the desktop
    {
        //Set window size
        m_rect.right = m_rect.left + m_window_width;
        m_rect.bottom = m_rect.top + m_window_height;

        if (force_adjust || m_rcMin.Width() != m_last_width)   //If the width changes, reposition the taskbar window
        {
            m_rcMinOri = m_rcMin;
            m_left_space = m_rcMin.left - m_rcBar.left;
            //Save the last width
            m_last_width = m_rcMin.Width() - m_rect.Width();
            //When the task window is displayed on the right
            if (!theApp.m_taskbar_data.tbar_wnd_on_left)
            {
                ::MoveWindow(m_hMin, m_left_space, 0, m_rcMin.Width() - m_rect.Width(), m_rcMin.Height(), TRUE);    //Set the position of the minimized window
                m_rect.MoveToX(m_left_space + m_rcMin.Width() - m_rect.Width() + 2);
            }
            //When the taskbar window is displayed on the left
            else
            {
                ::MoveWindow(m_hMin, m_left_space + m_rect.Width(), 0, m_rcMin.Width() - m_rect.Width(), m_rcMin.Height(), TRUE);
                m_rect.MoveToX(m_left_space);
            }

            //Set the vertical position of the taskbar window
            m_rect.MoveToY((m_rcBar.Height() - m_rect.Height()) / 2);
            if (theApp.m_taskbar_data.horizontal_arrange && theApp.m_win_version.IsWindows7())
                m_rect.MoveToY(m_rect.top + DPI(1));
            MoveWindow(m_rect);
        }
    }
    else        //When the taskbar is on the left or right side of the screen
    {
        //Set window size
        if (force_adjust || m_rcMin.Height() != m_last_height)     //If the height changes, reposition the taskbar window
        {
            m_rcMinOri = m_rcMin;
            m_top_space = m_rcMin.top - m_rcBar.top;
            //Save the last width
            m_last_height = m_rcMin.Height() - m_rect.Height();

            if (!theApp.m_taskbar_data.tbar_wnd_on_left)
            {
                ::MoveWindow(m_hMin, 0, m_top_space, m_rcMin.Width(), m_rcMin.Height() - m_rect.Height(), TRUE);    //Set the position of the minimized window
                m_rect.MoveToY(m_top_space + m_rcMin.Height() - m_rect.Height() + 2);
            }
            else
            {
                ::MoveWindow(m_hMin, 0, m_top_space + m_rect.Height(), m_rcMin.Width(), m_rcMin.Height() - m_rect.Height(), TRUE);  //Set the position of the minimized window
                m_rect.MoveToY(m_top_space);
            }
            m_rect.MoveToX((m_rcMin.Width() - m_window_width) / 2);
            int left_space = DPI(2);
            if (m_rect.left < left_space)
                m_rect.MoveToX(left_space);
            MoveWindow(m_rect);
        }
    }
}

void CClassicalTaskbarDlg::InitTaskbarWnd()
{
    m_hBar = ::FindWindowEx(m_hTaskbar, 0, L"ReBarWindow32", NULL); //Find the handle of the secondary container
    if (m_hBar == NULL)
        m_hBar = ::FindWindowEx(m_hTaskbar, nullptr, L"WorkerW", NULL);
    m_hMin = ::FindWindowEx(m_hBar, 0, L"MSTaskSwWClass", NULL);    //Find the handle of the minimized window
    if (m_hMin == NULL)
        m_hMin = ::FindWindowEx(m_hBar, 0, L"MSTaskListWClass", NULL);    //Find the handle of the minimized window

    ::GetWindowRect(m_hMin, m_rcMin);   //Get the area of the minimized window
    ::GetWindowRect(m_hBar, m_rcBar);   //Get the area of the secondary container

    m_left_space = m_rcMin.left - m_rcBar.left;
    m_top_space = m_rcMin.top - m_rcBar.top;
}

void CClassicalTaskbarDlg::ResetTaskbarPos()
{
    //When the program is closed, restore the width of the minimized window.
    if (!m_rcMinOri.IsRectEmpty())
    {
        if (m_taskbar_on_top_or_bottom)
            ::MoveWindow(m_hMin, m_left_space, 0, m_rcMinOri.Width(), m_rcMinOri.Height(), TRUE);
        else

            ::MoveWindow(m_hMin, 0, m_top_space, m_rcMinOri.Width(), m_rcMinOri.Height(), TRUE);
    }
}

HWND CClassicalTaskbarDlg::GetParentHwnd()
{
    return m_hBar;
}

void CClassicalTaskbarDlg::CheckTaskbarOnTopOrBottom()
{
    CRect rect;
    CRect rcMin;
    CRect rcBar;
    if (m_hTaskbar != 0)
    {
        //::GetWindowRect(m_hMin, rcMin); //Get the area of the minimized window
        //::GetWindowRect(m_hBar, rcBar); //Get the area of the secondary container
        //if (m_left_space == 0)
        //    m_left_space = rcMin.left - rcBar.left;
        //if (m_top_space == 0)
        //    m_top_space = rcMin.top - rcBar.top;

        ::GetWindowRect(m_hTaskbar, rect);          //Get the rectangular area of the taskbar
        m_taskbar_on_top_or_bottom = (rect.Width() >= rect.Height());     //If the taskbar is wider than it is tall, the task is at the top or bottom of the screen
    }
    else
    {
        m_taskbar_on_top_or_bottom = true;
    }
}

