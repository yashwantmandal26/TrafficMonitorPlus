#pragma once
#include "IniHelper.h"
#include "CommonData.h"

class CSettingsHelper : public CIniHelper
{
public:
    CSettingsHelper();
    CSettingsHelper(const std::wstring& file_path);

    void SaveFontData(const wchar_t* AppName, const FontInfo& font);
    void LoadFontData(const wchar_t* AppName, FontInfo& font, const FontInfo& default_font) const;

    /**
     * @brief Read the main window color settings
     * @param AppName[in] section of ini file
     * @param KeyName[in] key of ini file
     * @param text_colors[out] Save the read colors
     * @param default_color[in] default color
     */
    void LoadMainWndColors(const wchar_t* AppName, const wchar_t* KeyName, const std::set<CommonDisplayItem>& all_items, std::map<CommonDisplayItem, COLORREF>& text_colors, COLORREF default_color);
    
    /**
     * @brief saves the main window color settings
     * @param AppName segment of ini file
     * @param KeyName ini file key
     * @param text_colors The color to save
     */
    void SaveMainWndColors(const wchar_t* AppName, const wchar_t* KeyName, const std::map<CommonDisplayItem, COLORREF>& text_colors);

    void LoadTaskbarWndColors(const wchar_t* AppName, const wchar_t* KeyName, std::map<CommonDisplayItem, TaskbarItemColor>& text_colors, const wchar_t* default_str);
    void LoadTaskbarWndColors(const wchar_t* AppName, const wchar_t* KeyName, std::map<CommonDisplayItem, TaskbarItemColor>& text_colors, COLORREF default_color);
    void SaveTaskbarWndColors(const wchar_t* AppName, const wchar_t* KeyName, const std::map<CommonDisplayItem, TaskbarItemColor>& text_colors);

    void LoadDisplayStr(const wchar_t* AppName, DispStrings& disp_str, bool is_main_window) const;
    void SaveDisplayStr(const wchar_t* AppName, const DispStrings& disp_str);

    void LoadPluginDisplayStr(const wchar_t* AppName, DispStrings& disp_str, bool is_main_window);
    void SavePluginDisplayStr(const wchar_t* AppName, const DispStrings& disp_str);

};

