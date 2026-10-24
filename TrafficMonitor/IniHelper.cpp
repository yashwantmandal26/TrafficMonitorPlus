#include "stdafx.h"
#include "IniHelper.h"
#include "Common.h"

CIniHelper::CIniHelper(const wstring& file_path, bool force_utf8)
{
    m_file_path = file_path;
    ifstream file_stream{ file_path };
    if (!file_stream.is_open())
        return;
    // Get file size
    file_stream.seekg(0, std::ios::end);
    size_t file_size = static_cast<size_t>(file_stream.tellg());
    file_stream.seekg(0, std::ios::beg);
    // Read file contents
    string ini_str;
    ini_str.resize(file_size + 1);
    file_stream.read(&ini_str[0], file_size);
    // Check and add a blank line at the end
    if (!ini_str.empty() && ini_str.back() != L'\n')
        ini_str.push_back(L'\n');
    bool is_utf8;
    if (force_utf8)
    {
        is_utf8 = true;
    }
    else
    {
        //Determine whether the file is utf8 encoded
        if (ini_str.size() >= 3 && ini_str[0] == -17 && ini_str[1] == -69 && ini_str[2] == -65)
        {
            //If there is a UTF8 BOM, delete the BOM
            is_utf8 = true;
            ini_str = ini_str.substr(3);
        }
        else
        {
            is_utf8 = false;
        }
    }
    //Convert to Unicode
    m_ini_str = CCommon::StrToUnicode(ini_str.c_str(), is_utf8);
}

CIniHelper::CIniHelper(UINT id, bool is_utf8)
{
    m_ini_str = CCommon::GetTextResource(id, is_utf8 ? 1 : 0);
}

CIniHelper::CIniHelper()
{
}


CIniHelper::~CIniHelper()
{
}

void CIniHelper::FromDirectString(const wstring& str_content)
{
    m_ini_str = str_content;
}

void CIniHelper::SetSaveAsUTF8(bool utf8)
{
    m_save_as_utf8 = utf8;
}

void CIniHelper::WriteString(const wchar_t * AppName, const wchar_t * KeyName, const wstring& str)
{
    wstring write_str{ str };
    if (!write_str.empty() && (write_str[0] == L' ' || write_str.back() == L' '))       //If there are spaces before and after the string, add quotation marks before and after the string.
    {
        write_str = DEF_CH + write_str;
        write_str.push_back(DEF_CH);
    }
    _WriteString(AppName, KeyName, write_str);
}

wstring CIniHelper::GetString(const wchar_t * AppName, const wchar_t * KeyName, const wchar_t* default_str) const
{
    wstring rtn{ default_str };
    GetString(AppName, KeyName, rtn);
    return rtn;
}

bool CIniHelper::GetString(const wchar_t* AppName, const wchar_t* KeyName, wstring& str) const
{
    bool rtn = _GetString(AppName, KeyName, str);
    //If there are specified characters before and after the read string, delete it
    if (!str.empty() && (str.front() == L'$' || str.front() == DEF_CH))
        str = str.substr(1);
    if (!str.empty() && (str.back() == L'$' || str.back() == DEF_CH))
        str.pop_back();
    return rtn;
}

void CIniHelper::WriteInt(const wchar_t * AppName, const wchar_t * KeyName, int value)
{
    _WriteString(AppName, KeyName, std::to_wstring(value));
}

int CIniHelper::GetInt(const wchar_t * AppName, const wchar_t * KeyName, int default_value) const
{
    wstring rtn{ std::to_wstring(default_value) };
    _GetString(AppName, KeyName, rtn);
    return _ttoi(rtn.c_str());
}

void CIniHelper::WriteBool(const wchar_t * AppName, const wchar_t * KeyName, bool value)
{
    if(value)
        _WriteString(AppName, KeyName, wstring(L"true"));
    else
        _WriteString(AppName, KeyName, wstring(L"false"));
}

bool CIniHelper::GetBool(const wchar_t * AppName, const wchar_t * KeyName, bool default_value) const
{
    wstring rtn{ default_value ? L"true" : L"false" };
    _GetString(AppName, KeyName, rtn);
    if (rtn == L"true")
        return true;
    else if (rtn == L"false")
        return false;
    else
        return (_ttoi(rtn.c_str()) != 0);
}

void CIniHelper::WriteIntArray(const wchar_t * AppName, const wchar_t * KeyName, const int * values, int size)
{
    CString str, tmp;
    for (int i{}; i < size; i++)
    {
        tmp.Format(_T("%d,"), values[i]);
        str += tmp;
    }
    _WriteString(AppName, KeyName, wstring(str));
}

void CIniHelper::GetIntArray(const wchar_t * AppName, const wchar_t * KeyName, int * values, int size, int default_value) const
{
    CString default_str;
    default_str.Format(_T("%d"), default_value);
    wstring str{ default_str.GetString() };
    _GetString(AppName, KeyName, str);
    std::vector<wstring> split_result;
    CCommon::StringSplit(str, L',', split_result);
    for (int i = 0; i < size; i++)
    {
        if (i < split_result.size())
            values[i] = _wtoi(split_result[i].c_str());
        else if (i > 0)
            values[i] = values[i - 1];
        else
            values[i] = default_value;
    }
}

void CIniHelper::WriteBoolArray(const wchar_t * AppName, const wchar_t * KeyName, const bool * values, int size)
{
    int value{};
    for (int i{}; i < size; i++)
    {
        if (values[i])
            value |= (1 << i);
    }
    return WriteInt(AppName, KeyName, value);
}

void CIniHelper::GetBoolArray(const wchar_t * AppName, const wchar_t * KeyName, bool * values, int size, bool default_value) const
{
    int value = GetInt(AppName, KeyName, 0);
    for (int i{}; i < size; i++)
    {
        values[i] = ((value >> i) % 2 != 0);
    }
}

void CIniHelper::WriteStringList(const wchar_t* AppName, const wchar_t* KeyName, const vector<wstring>& values)
{
    wstring str_write = MergeStringList(values);
    _WriteString(AppName, KeyName, str_write);
}

void CIniHelper::GetStringList(const wchar_t* AppName, const wchar_t* KeyName, vector<wstring>& values, const vector<wstring>& default_value) const
{
    wstring str_value = MergeStringList(default_value);
    _GetString(AppName, KeyName, str_value);
    SplitStringList(values, str_value);
}

vector<wstring> CIniHelper::GetAllAppName(const wstring& prefix) const
{
    vector<wstring> list;
    size_t pos{};
    while ((pos = m_ini_str.find(L"\n[" + prefix, pos)) != wstring::npos)
    {
        size_t end = m_ini_str.find(L']', pos + 1);
        if (end != wstring::npos)
        {
            wstring tmp(m_ini_str.begin() + pos + prefix.size() + 2, m_ini_str.begin() + end);
            list.push_back(std::move(tmp));
            pos = end + 1;
        }
    }
    return list;
}

void CIniHelper::GetAllKeyValues(const wstring& AppName, std::map<wstring, wstring>& map) const
{
    wstring app_str{ L"[" };
    app_str.append(AppName).append(L"]");
    size_t app_pos{}, app_end_pos{};
    app_pos = m_ini_str.find(app_str);
    if (app_pos == wstring::npos)
        return;
    app_end_pos = m_ini_str.find(L"\n[", app_pos + 2);
    if (app_end_pos != wstring::npos)
        app_end_pos++;
    app_str = m_ini_str.substr(app_pos, app_end_pos - app_pos);
    vector<wstring> line;
    CCommon::StringSplit(app_str, L'\n', line);
    for (wstring str : line)
    {
        // CCommon::StringSplit will skip empty strings, str must be non-empty
        if (str[0] == L';' || str[0] == L'#')   // Skip comment lines (only first line comments are supported)
            continue;
        size_t pos = str.find_first_of(L'=');
        if (pos == wstring::npos)
            continue;
        wstring key{ str.substr(0, pos) };
        wstring value{ str.substr(pos + 1) };
        CCommon::StringNormalize(key);
        CCommon::StringNormalize(value);
        if (!key.empty() && !value.empty())
        {
            if (value.front() == L'\"' && value.back() == L'\"')
                value = value.substr(1, value.size() - 2);
            UnEscapeString(value);
            map[key] = value;
        }
    }
}

bool CIniHelper::RemoveSection(const wstring& AppName)
{
    if (AppName.empty())
        return false;
    wstring app_str{ L"[" };
    app_str.append(AppName).append(L"]");
    size_t app_pos{}, app_end_pos{};
    app_pos = m_ini_str.find(app_str);
    if (app_pos == wstring::npos)       //AppName not found, returns default string
        return false;

    app_end_pos = m_ini_str.find(L"\n[", app_pos + 2);
    if (app_end_pos != wstring::npos)
        app_end_pos++;

    m_ini_str.erase(app_pos, app_end_pos - app_pos);

    return true;
}

bool CIniHelper::Save()
{
    if (m_file_path.empty())
        return false;
    ofstream file_stream{ m_file_path };
    if(file_stream.fail())
        return false;
    string ini_str{ CCommon::UnicodeToStr(m_ini_str.c_str(), m_save_as_utf8) };
    if (m_save_as_utf8)     //If saved in UTF8 encoding, insert BOM first
    {
        string utf8_bom;
        utf8_bom.push_back(-17);
        utf8_bom.push_back(-69);
        utf8_bom.push_back(-65);
        file_stream << utf8_bom;
    }

    file_stream << ini_str;
    return true;
}

void CIniHelper::UnEscapeString(wstring& str)
{
    bool escape{ false };
    wstring result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); i++)
    {
        wchar_t ch = str[i];
        if (escape)
        {
            switch (ch)
            {
            case L'n': result += L'\n'; break;
            case L'r': result += L'\r'; break;
            case L't': result += L'\t'; break;
            case L'"': result += L'"'; break;
            case L';': result += L';'; break;
            case L'#': result += L'#'; break;
            case L'\\': result += L'\\'; break;
            default:result += '\\'; result += ch; break;
            }
            escape = false;
        }
        else if (ch == L'\\')
            escape = true;
        else if (i > 0 && ch == '\"' && str[i - 1] == '\"')     //Two consecutive quotes keep only one quote
            continue;
        else
            result += ch;
    }
    str.swap(result);
}

void CIniHelper::_WriteString(const wchar_t * AppName, const wchar_t * KeyName, const wstring & str)
{
    wstring app_str{ L"[" };
    app_str.append(AppName).append(L"]");
    size_t app_pos{}, app_end_pos, key_pos;
    app_pos = m_ini_str.find(app_str);
    if (app_pos == wstring::npos)       //If AppName cannot be found, add it at the end
    {
        if (!m_ini_str.empty() && m_ini_str.back() != L'\n')
            m_ini_str += L"\n";
        app_pos = m_ini_str.size();
        m_ini_str += app_str;
        m_ini_str += L"\n";
    }
    app_end_pos = m_ini_str.find(L"\n[", app_pos + 2);
    if (app_end_pos != wstring::npos)
        app_end_pos++;

    key_pos = m_ini_str.find(wstring(L"\n") + KeyName + L' ', app_pos);     //Find "\nkey_name"
    if (key_pos >= app_end_pos)     //If "\nkey_name " is not found, then look for "\nkey_name="
        key_pos = m_ini_str.find(wstring(L"\n") + KeyName + L'=', app_pos);
    if (key_pos >= app_end_pos)             //If KeyName is not found, insert one
    {
        //wchar_t buff[256];
        //swprintf_s(buff, L"%s = %s\n", KeyName, str.c_str());
        std::wstring str_temp = KeyName;
        str_temp += L" = ";
        str_temp += str;
        str_temp += L"\n";
        if (app_end_pos == wstring::npos)
            m_ini_str += str_temp;
        else
            m_ini_str.insert(app_end_pos, str_temp);
    }
    else    //KeyName is found, replace the text between the equal sign and the newline character
    {
        size_t str_pos;
        str_pos = m_ini_str.find(L'=', key_pos + 2);
        size_t line_end_pos = m_ini_str.find(L'\n', key_pos + 2);
        if (str_pos > line_end_pos) //If there is no equal sign on the line, insert an equal sign.
        {
            m_ini_str.insert(key_pos + wcslen(KeyName) + 1, L" =");
            str_pos = key_pos + wcslen(KeyName) + 2;
        }
        else
        {
            str_pos++;
        }
        size_t str_end_pos;
        str_end_pos = m_ini_str.find(L"\n", str_pos);
        m_ini_str.replace(str_pos, str_end_pos - str_pos, L" " + str);
    }
}

bool CIniHelper::_GetString(const wchar_t* AppName, const wchar_t* KeyName, wstring& str) const
{
    wstring app_str{ L"[" };
    app_str.append(AppName).append(L"]");
    size_t app_pos{}, app_end_pos, key_pos;
    app_pos = m_ini_str.find(app_str);
    if (app_pos == wstring::npos)       //AppName not found, returns default string
        return false;

    app_end_pos = m_ini_str.find(L"\n[", app_pos + 2);
    if (app_end_pos != wstring::npos)
        app_end_pos++;

    key_pos = m_ini_str.find(wstring(L"\n") + KeyName + L' ', app_pos);     //Find "\nkey_name"
    if (key_pos >= app_end_pos)     //If "\nkey_name " is not found, then look for "\nkey_name="
        key_pos = m_ini_str.find(wstring(L"\n") + KeyName + L'=', app_pos);
    if (key_pos >= app_end_pos)             //KeyName not found, returns default string
    {
        return false;
    }
    else    //KeyName is found, get the text between the equal sign and the newline character
    {
        size_t str_pos;
        str_pos = m_ini_str.find(L'=', key_pos + 2);
        size_t line_end_pos = m_ini_str.find(L'\n', key_pos + 2);
        if (str_pos > line_end_pos) //If there is no equal sign in the line, the default string is returned.
        {
            return false;
        }
        else
        {
            str_pos++;
        }
        size_t str_end_pos;
        str_end_pos = m_ini_str.find(L"\n", str_pos);
        //Get text
        wstring return_str{ m_ini_str.substr(str_pos, str_end_pos - str_pos) };
        //If there are spaces before and after, remove them
        CCommon::StringNormalize(return_str);
        str = return_str;
        return true;
    }
}

wstring CIniHelper::MergeStringList(const vector<wstring>& values)
{
    wstring str_merge;
    int index = 0;
    //Put quotes around each string and concatenate them with commas
    for (const wstring& str : values)
    {
        if (index > 0)
            str_merge.push_back(L',');
        str_merge.push_back(L'\"');
        str_merge += str;
        str_merge.push_back(L'\"');
        index++;
    }
    return str_merge;
}

void CIniHelper::SplitStringList(vector<wstring>& values, const wstring& str_value)
{
    CCommon::StringSplit(str_value, wstring(L"\",\""), values);
    if (!values.empty())
    {
        //There are quotation marks before the first item and after the last item in the result. Delete them.
        values.front() = values.front().substr(1);
        values.back().pop_back();
    }
}
