
// stdafx.h: include file for standard system include files,
// Or frequently used but infrequently changed
// Project-specific include files

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely used data from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // Some CString constructors will be explicit

// Turn off MFC's hiding of some common but often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core components and standard components
#include <afxext.h>         // MFC extension


#include <afxdisp.h>        // MFC automation class



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 common controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows common controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC support for ribbons and control strips

#include "resource.h"
#include <string>
using std::string;
using std::wstring;
#include <vector>
using std::vector;
#include <deque>
using std::deque;
#include <list>
using std::list;
#include<iostream>
#include<fstream>
using std::ifstream;
using std::ofstream;
#include <map>
#include <set>
#include<io.h>
#include<algorithm>
#include <cmath>
#include <afxinet.h>    //Used to support the use of network-related classes
#include <afxwin.h>

#include "language.h"

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


#define MY_WM_NOTIFYICON (WM_USER+1005)
#define WM_TASKBAR_WND_CLOSED (WM_USER+1006)        //This message is sent when the taskbar window is closed
#define WM_MONITOR_INFO_UPDATED (WM_USER+1007)        //Monitoring information has been updated
#define WM_REOPEN_TASKBAR_WND (WM_USER+1008)        //Reopen the taskbar window
#define WM_SETTINGS_APPLIED (WM_USER+1009)          //Clicked the "Apply" button in the options settings

#define WM_NEXT_USER_MSG (WM_USER+1011)

//#define CONFIG_PATH _T(".\\config.ini")
//#define CONFIG_PATHA ".\\config.ini"
//#define LOG_PATH _T(".\\error.log")
//#define HISTORY_TRAFFIC_PATH _T(".\\history_traffic.dat")
#define MAX_RESTART_CNT 10      //Maximum number of reinitializations

#define MAIN_TIMER 1234         //The ID of the main timer
#define DELAY_TIMER 1235        //Delay timer ID
#define TASKBAR_TIMER 1236
#define CONNECTION_DETAIL_TIMER 1237
#define MONITOR_TIMER 1238
#define DELETE_NOTIFY_ICON_TIMER 1239   //Delete timer for notification area icon
#define RESTART_TASKBAR_TIMER 1240
#define INIT_CONNECT_TIMER 1241
#define DPI_CHANGE_TIMER 1242

#define MAX_INSERT_TO_TASKBAR_CNT 200     //Maximum number of attempts to embed the taskbar
#define WARN_INSERT_TO_TASKBAR_CNT 20     //Number of warning attempts to embed in the taskbar

#define APP_NAME _T("TrafficMonitor")
#define TASKBAR_WINDOW_NAME _T("TrafficMonitorTaskbarWindow")
#define APP_CLASS_NAME _T("TrafficMonitor_r7XZaS4p") //The class name of the program's main window
#define VERSION L"1.86"
#define COPYRITE_YEAR L"2017-2026"

#define MAX_NOTIFY_ICON 6       //Optional number of notification area icons

//The file name of the background image
#define BACKGROUND_IMAGE_S L"\\background.bmp"
#define BACKGROUND_IMAGE_L L"\\background_l.bmp"
#define BACKGROUND_IMAGE_S_PNG L"background.png"
#define BACKGROUND_IMAGE_L_PNG L"background_l.png"
#define BACKGROUND_MASK_S L"\\background_mask.bmp"
#define BACKGROUND_MASK_L L"\\background_mask_l.bmp"

#define MIN_FONT_SIZE 5
#define MAX_FONT_SIZE 72

#define SAFE_DELETE(p) do \
{\
    if(p != nullptr) \
    { \
        delete p; \
        p = nullptr; \
    } \
} while (false)

//Define two colors used as the transparent color of the taskbar (used when you need to set the taskbar window to be opaque)
#define TASKBAR_TRANSPARENT_COLOR1 RGB(52, 28, 41)
#define TASKBAR_TRANSPARENT_COLOR2 RGB(38, 67, 55)

//If you need to compile for Windows XP system, please remove the comment of the following line of code
//#define COMPILE_FOR_WINXP

//Whether to disable the WebExperience detection function, which is used to detect whether there are "widgets" under Windows 11
//If this part of the function causes compilation errors, please remove the comment on the following line of code
//#define DISABLE_WINDOWS_WEB_EXPERIENCE_DETECTOR
