#include "stdafx.h"
#include "StrTable.h"
#include "Common.h"
#include "IniHelper.h"
#include "TrafficMonitor.h"
CStrTable::CStrTable()
{
}

CStrTable::~CStrTable()
{
}

static void LanguageInfoFromIni(LanguageInfo& language_info, const CIniHelper& ini)
{
    language_info.display_name = ini.GetString(L"general", L"DISPLAY_NAME", L"");
    language_info.bcp_47 = ini.GetString(L"general", L"BCP_47", L"");
    language_info.default_font_name = ini.GetString(L"general", L"DEFAULT_FONT", L"Microsoft Sans Serif");
    language_info.translator = ini.GetString(L"general", L"TRANSLATOR", L"<Unknown>");
    language_info.translator_url = ini.GetString(L"general", L"TRANSLATOR_URL", L"");
}

// Callback function for enumerating resource languages
static BOOL CALLBACK EnumResLangProc(HMODULE hModule, LPCTSTR lpType, LPCTSTR lpName, WORD wIDLanguage, LONG_PTR lParam)
{
    std::vector<LanguageInfo>* pLanguages = reinterpret_cast<std::vector<LanguageInfo>*>(lParam);

    // Get resource handle
    HRSRC hRes = FindResourceEx(hModule, lpType, lpName, wIDLanguage);
    if (hRes != NULL)
    {
        // Load resources
        HGLOBAL hGlobal = LoadResource(hModule, hRes);
        if (hGlobal != NULL)
        {
            // Lock the resource and get the data pointer
            LPVOID pResourceData = LockResource(hGlobal);
            if (pResourceData != NULL)
            {
                // Get resource size
                DWORD resSize = SizeofResource(hModule, hRes);

                // Convert resource content to string
                std::string strData((const char*)pResourceData, resSize);
                std::wstring resData = CCommon::StrToUnicode(strData.c_str(), true).c_str();

                CIniHelper ini;
                ini.FromDirectString(resData);
                LanguageInfo lanugage_info;
                LanguageInfoFromIni(lanugage_info, ini);
                lanugage_info.language_id = wIDLanguage;
                pLanguages->push_back(lanugage_info);
            }
        }
    }

    return TRUE; // Continue enumeration
}


void CStrTable::Init()
{
    // Get all language versions of the IDR_LANGUAGE resource
    EnumResourceLanguages(NULL, _T("TEXT"), MAKEINTRESOURCE(IDR_LANGUAGE), EnumResLangProc, reinterpret_cast<LONG_PTR>(&m_language_list));
    //Sort by bcp47
    std::sort(m_language_list.begin(), m_language_list.end(), [](const LanguageInfo& a, const LanguageInfo& b) {
        return a.bcp_47 < b.bcp_47;
    });

    //First read the default string resource. If there is no corresponding string in the current language, the default string resource will be used.
    CIniHelper ini_default(IDR_LANGUAGE_DEFAULT);
    ReadStringtableFronIni(ini_default);

    //Read string resources
    CIniHelper ini(IDR_LANGUAGE);
    ReadStringtableFronIni(ini);
    LanguageInfoFromIni(m_language_info, ini);

    //Get the current system default language
    DWORD cur_system_language_id = GetUserDefaultUILanguage();

    //Get language files from external language folder
    vector<wstring> files;
    std::wstring language_dir;
#ifdef _DEBUG
    language_dir = L".\\language";
#else
    language_dir = theApp.m_module_dir + L"language";
#endif
    CCommon::GetFiles((language_dir + L"\\*.ini").c_str(), files);
    for (const wstring& file_name : files)
    {
        std::wstring file_path{ language_dir + file_name };
        CIniHelper ini_file(file_path, true);
        LanguageInfo language_info;
        LanguageInfoFromIni(language_info, ini_file);
        language_info.language_id = LocaleNameToLCID(language_info.bcp_47.c_str(), 0);  //Get language id based on language bcp-47 code
        //Read from an external language file to the current language, first load from the external language file
        bool is_current_language = false;
        LanguageInfo settings_language = theApp.m_general_data.language;
        //If the language in the settings is empty, that is, "Follow system" is selected, the search will be based on the current system language.
        if (settings_language.isEmpty())
        {
            if (cur_system_language_id == language_info.language_id)
                is_current_language = true;
            else if (language_info.bcp_47 == L"en-US")
                is_current_language = true;
        }
        else
        {
            if (language_info == settings_language)
                is_current_language = true;
        }
        if (is_current_language)
        {
            m_language_info = language_info;
            ReadStringtableFronIni(ini_file);
        }

        //If the language is not in m_language_list, add it to the list
        auto iter = std::find(m_language_list.begin(), m_language_list.end(), language_info);
        if (iter == m_language_list.end())
            m_language_list.push_back(language_info);
    }
}

const wstring& CStrTable::LoadText(const wstring& key) const
{
    const auto& text_string_table = GetTextStringTable();
    return LoadText(key, text_string_table);
}

wstring CStrTable::LoadTextFormat(const wstring& key, const std::initializer_list<CVariant>& paras) const
{
    const auto& text_string_table = GetTextStringTable();
    return LoadTextFormat(key, text_string_table, paras);
}

const wstring& CStrTable::LoadMenuText(const wstring& key) const
{
    const auto& menu_string_table = GetMenuStringTable();
    return LoadText(key, menu_string_table);
}

const wstring& CStrTable::LoadText(const wstring& key, const wstring& section)
{
    auto iter = m_string_table.find(section);
    if (iter != m_string_table.end())
    {
        return LoadText(key, iter->second);
    }
    static std::wstring str_empty;
    return str_empty;
}

void CStrTable::ReadStringtableFronIni(const CIniHelper& ini)
{
    std::set<std::wstring> sections{ L"general", L"text", L"menu" };
    auto section_list = ini.GetAllAppName(L"");
    for (const auto& section : section_list)
        sections.insert(section);

    for (const auto& section : sections)
    {
        auto& value_map = m_string_table[section];
        ini.GetAllKeyValues(section, value_map);
    }
}

const std::map<std::wstring, std::wstring>& CStrTable::GetTextStringTable() const
{
    auto iter = m_string_table.find(L"text");
    if (iter != m_string_table.end())
        return iter->second;
    static std::map<std::wstring, std::wstring> empty_map;
    return empty_map;
}

const std::map<std::wstring, std::wstring>& CStrTable::GetMenuStringTable() const
{
    auto iter = m_string_table.find(L"menu");
    if (iter != m_string_table.end())
        return iter->second;
    static std::map<std::wstring, std::wstring> empty_map;
    return empty_map;
}

const wstring& CStrTable::LoadText(const wstring& key, const std::map<std::wstring, std::wstring>& string_table)
{
    auto iter = string_table.find(key);
    if (iter != string_table.end())
        return iter->second;
    ASSERT(false);
    static std::wstring str_empty;
    return str_empty;
}

wstring CStrTable::LoadTextFormat(const wstring& key, const std::map<std::wstring, std::wstring>& string_table, const std::initializer_list<CVariant>& paras)
{
    auto iter = string_table.find(key);
    if (iter == string_table.end())
    {
        ASSERT(false);
        return std::wstring();
    }
    wstring str{ iter->second };    // Copy to avoid original string modification
    int index{ 1 };
    for (const auto& para : paras)
    {
        wstring format_str{ L"<%" + std::to_wstring(index) + L"%>" };
        CCommon::StringReplace(str, format_str, para.ToString().GetString());
        ++index;
    }
    return str;
}
