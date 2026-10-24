#include "stdafx.h"
#include "TaskbarHelper.h"
#include <algorithm>

// Store display information
struct MonitorInfo
{
    HMONITOR hMonitor;
    CRect rect;
};

// Store taskbar information
struct TaskbarInfo
{
    HWND hwnd;
    CRect rect;
};

std::vector<MonitorInfo> monitors;
std::vector<TaskbarInfo> taskbars;

// Callback function for enumeration displays
static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    MonitorInfo info;
    info.hMonitor = hMonitor;
    info.rect = *lprcMonitor;
    if (!info.rect.IsRectEmpty())
        monitors.push_back(info);
    else
        ASSERT(FALSE);
    return TRUE;
}

// Enumeration window callback function
static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    TCHAR className[256];
    GetClassName(hwnd, className, 256);

    // Check if it is the taskbar of the secondary monitor
    if (_tcscmp(className, _T("Shell_SecondaryTrayWnd")) == 0)
    {
        TaskbarInfo info;
        info.hwnd = hwnd;
        GetWindowRect(hwnd, &info.rect);
        taskbars.push_back(info);
    }

    return TRUE;
}

// Comparison function: Sort by the coordinates of the upper left corner of the display
static bool CompareTaskbarByMonitorOrder(const TaskbarInfo& a, const TaskbarInfo& b)
{
    // Traverse all displays
    for (const auto& monitor : monitors)
    {
        // Check whether taskbar a is within the current monitor
        bool aInMonitor = (a.rect.left >= monitor.rect.left && a.rect.top >= monitor.rect.top);
        // Check whether taskbar b is within the current monitor
        bool bInMonitor = (b.rect.left >= monitor.rect.left && b.rect.top >= monitor.rect.top);

        // If a is in the current display but b is not, then a should be in front of b
        if (aInMonitor && !bInMonitor)
            return true;
        // If b is in the current display and a is not, then b should be in front of a
        if (bInMonitor && !aInMonitor)
            return false;
        // If a and b are both within the current monitor, sort by taskbar position
        if (aInMonitor && bInMonitor)
        {
            if (a.rect.left != b.rect.left)
                return a.rect.left < b.rect.left;
            return a.rect.top < b.rect.top;
        }
    }
    // By default, a and b are equal
    return false;
}

void CTaskbarHelper::GetAllSecondaryDisplayTaskbar(std::vector<HWND>& secondary_taskbars)
{
    monitors.clear();
    taskbars.clear();
    secondary_taskbars.clear();

    // Get all monitor information
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

    // Get all taskbar handles
    EnumWindows(EnumWindowsProc, 0);

    // Sort taskbar in monitor order
    std::sort(taskbars.begin(), taskbars.end(), CompareTaskbarByMonitorOrder);

    //Save taskbar handle
    for (const auto& taskbar : taskbars)
    {
        secondary_taskbars.push_back(taskbar.hwnd);
    }
}

int CTaskbarHelper::GetDisplayNum()
{
    monitors.clear();
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
    return static_cast<int>(monitors.size());
}

int CTaskbarHelper::GetSecondaryTaskbarNum()
{
    taskbars.clear();
    EnumWindows(EnumWindowsProc, 0);
    return static_cast<int>(taskbars.size());
}
