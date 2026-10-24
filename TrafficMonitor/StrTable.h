#pragma once
#include "CVariant.h"
#include <map>
#include "CommonData.h"
class CIniHelper;

class CStrTable
{
public:
    CStrTable();
    ~CStrTable();

    void Init();

    // Load translation string
    const wstring& LoadText(const wstring& key) const;
    // Load the translation string and replace the string in the form of <% sequence number%> with the parameters in the variable parameter list
    wstring LoadTextFormat(const wstring& key, const std::initializer_list<CVariant>& paras) const;

    const wstring& LoadMenuText(const wstring& key) const;

    const wstring& LoadText(const wstring& key, const wstring& section);

    // Get the default font name
    const LanguageInfo& GetLanguageInfo() const { return m_language_info; }

    // Get all supported languages
    const std::vector<LanguageInfo>& GetLanguageList() const { return m_language_list; }

    // Whether the current language is Simplified Chinese (select the default update source and update information language)
    bool IsSimplifiedChinese() const { return  m_language_info.bcp_47 == L"zh-CN"; };

protected:
    void ReadStringtableFronIni(const CIniHelper& ini);
    const std::map<std::wstring, std::wstring>& GetTextStringTable() const;
    const std::map<std::wstring, std::wstring>& GetMenuStringTable() const;

    static const wstring& LoadText(const wstring& key, const std::map<std::wstring, std::wstring>& string_table);
    static wstring LoadTextFormat(const wstring& key, const std::map<std::wstring, std::wstring>& string_table, const std::initializer_list<CVariant>& paras);


private:
    std::map<std::wstring, std::map<std::wstring, std::wstring>> m_string_table;    //Save the key-value pairs of each section of the ini file
    LanguageInfo m_language_info;                   //Current language
    std::vector<LanguageInfo> m_language_list;      //All supported languages
};

