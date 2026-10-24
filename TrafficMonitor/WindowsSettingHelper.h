#pragma once
class CWindowsSettingHelper
{
public:
    static bool IsWindows10LightTheme();				//Determine whether Windows has a light theme
    static void CheckWindows10LightTheme();
    static bool IsDotNetFramework4Point5Installed();	//Determine whether .Net Framework 4.5 is installed (https://docs.microsoft.com/zh-cn/dotnet/framework/migration-guide/how-to-determine-which-versions-are-installed)
    static bool IsTaskbarShowingInAllDisplays();        //Whether the taskbar is shown on all monitors

    //Windows 11 related settings
    static bool IsTaskbarWidgetsBtnShown();             //Whether the taskbar displays the Widgets button
    static bool IsTaskbarCenterAlign();                  //Is the taskbar centered?

private:
    static LONG GetDWORDRegKeyData(HKEY hKey, const wstring& strValueName, DWORD& dwValueData);
    static bool GetDWORDRegKeyData(HKEY keyParent, const wstring& strKeyName, const wstring& strValueName, DWORD& dwValueData);

private:
    static 	bool m_light_theme;
};

