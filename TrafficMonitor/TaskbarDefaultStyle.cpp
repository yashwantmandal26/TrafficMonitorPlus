#include "stdafx.h"
#include "TaskbarDefaultStyle.h"
#include "SettingsHelper.h"
#include "TrafficMonitor.h"
#include "WindowsSettingHelper.h"

CTaskbarDefaultStyle::CTaskbarDefaultStyle()
{
}

CTaskbarDefaultStyle::~CTaskbarDefaultStyle()
{
	SaveConfig();
}

void CTaskbarDefaultStyle::LoadConfig()
{
    CSettingsHelper ini;
    for (int i = 0; i < TASKBAR_DEFAULT_STYLE_NUM; i++)
	{
        COLORREF default_text_color = (TASKBAR_DEFAULT_LIGHT_STYLE(i) ? RGB(0, 0, 0) : RGB(255, 255, 255));
        COLORREF darkmode_default_back_color = 0;
        if (theApp.m_win_version.IsWindows8Or8point1())
            darkmode_default_back_color = RGB(0, 0, 1);
        COLORREF default_back_color = (TASKBAR_DEFAULT_LIGHT_STYLE(i) ? RGB(240, 240, 240) : darkmode_default_back_color);
        COLORREF default_transparent_color = (TASKBAR_DEFAULT_LIGHT_STYLE(i) ? RGB(240, 240, 240) : darkmode_default_back_color);
        COLORREF default_status_bar_color = (TASKBAR_DEFAULT_LIGHT_STYLE(i) ? RGB(165, 165, 165) : 0x005A5A5A);
        wchar_t buff[64];
        swprintf_s(buff, L"default%d_", i + 1);
        wstring key_name = buff;

        bool specify_each_item_color_default = false;
        if (i == 0) // Style 1: Stealth Minimal Dark
        {
            ini.LoadTaskbarWndColors(L"taskbar_default_style", (key_name + L"text_color").c_str(), m_default_style[i].text_colors, default_text_color);
        }
        else if (i == 1) // Style 2: Pastel Sakura (Cute)
        {
            const wchar_t* str_style_sakura = L"11823615,12695295,13134010,15187681,6654719,12373247,14798925,15920050,5232127,12909055,4419839,9546751,9593584,13679608,13134010,15187681,14798925,15920050,13134010,16115187,11823615,12695295,8701825,13231816,11823615,15525116,";
            ini.LoadTaskbarWndColors(L"taskbar_default_style", (key_name + L"text_color").c_str(), m_default_style[i].text_colors, str_style_sakura);
            specify_each_item_color_default = true;
            default_status_bar_color = RGB(255, 182, 193);
        }
        else if (i == 2) // Style 3: Cyberpunk Neon (Funky)
        {
            const wchar_t* str_style_cyber = L"8388863,16777215,16773120,16777215,1376057,16777215,55295,16777215,16711860,16777215,24319,65535,3932415,11822335,16711860,16777215,16773120,16777215,55295,16777215,16773120,8388863,1376057,16777215,8388863,55295,";
            ini.LoadTaskbarWndColors(L"taskbar_default_style", (key_name + L"text_color").c_str(), m_default_style[i].text_colors, str_style_cyber);
            specify_each_item_color_default = true;
            default_status_bar_color = RGB(255, 0, 128);
        }
        else if (i == 3) // Style 4: Matcha Mint & Boba (Cute)
        {
            const wchar_t* str_style_matcha = L"10025880,10942426,12897614,16120294,10869155,15202814,7327988,15202814,12770930,16120294,9083647,12247039,9208575,13161215,12897614,16120294,12770930,16120294,7327988,15202814,10025880,10942426,10869155,10942426,12897614,7327988,";
            ini.LoadTaskbarWndColors(L"taskbar_default_style", (key_name + L"text_color").c_str(), m_default_style[i].text_colors, str_style_matcha);
            specify_each_item_color_default = true;
            default_status_bar_color = RGB(78, 205, 196);
        }
        else if (i == 4) // Style 5: Sunset Peach (Funky)
        {
            const wchar_t* str_style_sunset = L"5275647,11855359,50175,7202559,7039999,8036607,14706931,15780095,1744895,7202559,4935147,12235775,7239935,12500735,14706931,15780095,50175,7202559,5275647,11855359,5275647,50175,1744895,11855359,14706931,50175,";
            ini.LoadTaskbarWndColors(L"taskbar_default_style", (key_name + L"text_color").c_str(), m_default_style[i].text_colors, str_style_sunset);
            specify_each_item_color_default = true;
            default_status_bar_color = RGB(255, 127, 80);
        }
        else if (i == 5) // Style 6: Retro Synthwave (Funky)
        {
            const wchar_t* str_style_synthwave = L"16505672,16777215,8481791,16777215,11950491,15835354,10604829,15138740,5753598,11004671,5720063,11837695,14495422,16505672,11950491,15835354,16505672,16777215,10604829,15138740,16505672,8481791,5753598,11004671,8481791,5753598,";
            ini.LoadTaskbarWndColors(L"taskbar_default_style", (key_name + L"text_color").c_str(), m_default_style[i].text_colors, str_style_synthwave);
            specify_each_item_color_default = true;
            default_status_bar_color = RGB(155, 89, 182);
        }
        else if (i == 6) // Style 7: Arctic Glacial Blue (Cute)
        {
            const wchar_t* str_style_arctic = L"16641223,16777215,16753008,16777215,15553107,16768200,16121726,16777215,16768024,16775900,16602848,16765680,16765440,16775910,16753008,16777215,16641223,16777215,16121726,16777215,16641223,16753008,15553107,16768200,16602848,16121726,";
            ini.LoadTaskbarWndColors(L"taskbar_default_style", (key_name + L"text_color").c_str(), m_default_style[i].text_colors, str_style_arctic);
            specify_each_item_color_default = true;
            default_status_bar_color = RGB(24, 220, 255);
        }
        else if (i == 7) // Style 8: Clean Modern Light
        {
            const wchar_t* str_style_light = L"9863680,0,13785620,0,4623380,0,10496120,0,25790,0,1973940,0,6558880,0,10496120,0,9863680,0,25790,0,9863680,13785620,4623380,0,6558880,9863680,";
            ini.LoadTaskbarWndColors(L"taskbar_default_style", (key_name + L"text_color").c_str(), m_default_style[i].text_colors, str_style_light);
            specify_each_item_color_default = true;
            default_status_bar_color = RGB(165, 165, 165);
        }
        m_default_style[i].back_color = ini.GetInt(L"taskbar_default_style", (key_name + L"back_color").c_str(), default_back_color);
        m_default_style[i].transparent_color = ini.GetInt(L"taskbar_default_style", (key_name + L"transparent_color").c_str(), default_transparent_color);
        m_default_style[i].status_bar_color = ini.GetInt(L"taskbar_default_style", (key_name + L"status_bar_color").c_str(), default_status_bar_color);
        m_default_style[i].specify_each_item_color = ini.GetBool(L"taskbar_default_style", (key_name + L"specify_each_item_color").c_str(), specify_each_item_color_default);
	}
}

void CTaskbarDefaultStyle::SaveConfig() const
{
    CSettingsHelper ini;
    for (int i = 0; i < TASKBAR_DEFAULT_STYLE_NUM; i++)
	{
		wchar_t buff[64];
		swprintf_s(buff, L"default%d_", i + 1);
		wstring key_name = buff;
        if (IsTaskBarStyleDataValid(m_default_style[i]))           //Check whether the current color preset is valid before saving
        {
            ini.SaveTaskbarWndColors(L"taskbar_default_style", (key_name + L"text_color").c_str(), m_default_style[i].text_colors);
            ini.WriteInt(L"taskbar_default_style", (key_name + L"back_color").c_str(), m_default_style[i].back_color);
            ini.WriteInt(L"taskbar_default_style", (key_name + L"transparent_color").c_str(), m_default_style[i].transparent_color);
            ini.WriteInt(L"taskbar_default_style", (key_name + L"status_bar_color").c_str(), m_default_style[i].status_bar_color);
            ini.WriteBool(L"taskbar_default_style", (key_name + L"specify_each_item_color").c_str(), m_default_style[i].specify_each_item_color);
        }
        else
        {
            //write log
            CString log_str;
            log_str.Format(_T("When saving preset %d, background color and text color were both black. The preset was not saved."), i);
            CCommon::WriteLog(log_str, theApp.m_log_path.c_str());
            return;
        }
	}
	ini.Save();
}

void CTaskbarDefaultStyle::ApplyDefaultStyle(int index, TaskBarSettingData & data) const
{
    /*if (index == TASKBAR_DEFAULT_LIGHT_STYLE_INDEX)
    {
        ApplyDefaultLightStyle(data);
    }
    else */if (index >= 0 && index < TASKBAR_DEFAULT_STYLE_NUM)
	{
        if (!IsTaskBarStyleDataValid(m_default_style[index]))
            return;

        data.text_colors = m_default_style[index].text_colors;
		data.back_color = m_default_style[index].back_color;
		data.transparent_color = m_default_style[index].transparent_color;
		data.status_bar_color = m_default_style[index].status_bar_color;
		data.specify_each_item_color = m_default_style[index].specify_each_item_color;
		if (data.transparent_color == data.back_color)
		{
			CCommon::TransparentColorConvert(data.back_color);
			CCommon::TransparentColorConvert(data.transparent_color);
		}
	}
}

//void CTaskbarDefaultStyle::ApplyDefaultLightStyle(TaskBarSettingData& data)
//{
//	for (auto& item : data.text_colors)
//    {
//        item.second.label = RGB(0, 0, 0);
//        item.second.value = RGB(0, 0, 0);
//    }
//	data.back_color = RGB(210, 210, 211);
//	data.transparent_color = RGB(210, 210, 211);
//	data.status_bar_color = RGB(165, 165, 165);
//}

void CTaskbarDefaultStyle::ModifyDefaultStyle(int index, TaskBarSettingData & data)
{
	if (index < 0 || index >= TASKBAR_DEFAULT_STYLE_NUM)
		return;

    m_default_style[index].text_colors = data.text_colors;
	m_default_style[index].back_color = data.back_color;
	m_default_style[index].transparent_color = data.transparent_color;
	m_default_style[index].status_bar_color = data.status_bar_color;
	m_default_style[index].specify_each_item_color = data.specify_each_item_color;
}

bool CTaskbarDefaultStyle::IsTaskBarStyleDataValid(const TaskBarStyleData& data)
{
    for (const auto& item : data.text_colors)
    {
        if (item.second.label != data.back_color || item.second.value != data.back_color)
            return true;
    }
    return false;     //If the text colors are all equal to the background color, the color preset has no effect
}
