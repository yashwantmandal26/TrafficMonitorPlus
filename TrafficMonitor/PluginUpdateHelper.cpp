#include "stdafx.h"
#include "PluginUpdateHelper.h"
#include "Common.h"
#include "TrafficMonitor.h"
#include "TinyXml2Helper.h"

static void VersionFromString(const std::string& version_str, std::vector<int>& versions)
{
    versions.clear();
    //split string
    std::vector<std::string> vec_version;
    CCommon::StringSplit(version_str, '.', vec_version);
    //Convert to integer and save
    for (const auto& str : vec_version)
    {
        //Remove characters other than numbers
        std::string str_ver;
        for (auto ch : str)
        {
            if (ch >= '0' && ch <= '9')
                str_ver.push_back(ch);
        }

        versions.push_back(atoi(str_ver.c_str()));
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
PluginVersion::PluginVersion()
{
}

PluginVersion::PluginVersion(const std::wstring& version)
{
    m_version_str = CCommon::AsciiToStr(version);
    VersionFromString(m_version_str, m_version);
}

PluginVersion::PluginVersion(const std::string& version)
{
    m_version_str = version;
    VersionFromString(m_version_str, m_version);
}

bool PluginVersion::operator<(const PluginVersion& another) const
{
    for (size_t i{}; i < m_version.size() || i < another.m_version.size(); i++)
    {
        const int lhs = GetSubVersion(i);
        const int rhs = another.GetSubVersion(i);

        if (lhs < rhs)
            return true;

        if (lhs > rhs)
            return false;
    }
    return false;
}

bool PluginVersion::operator==(const PluginVersion& another) const
{
    for (size_t i{}; i < m_version.size() || i < another.m_version.size(); i++)
    {
        if (GetSubVersion(i) != another.GetSubVersion(i))
            return false;
    }
    return true;
}

std::string PluginVersion::GetVersionString() const
{
    return m_version_str;
}

std::wstring PluginVersion::GetVersionWString() const
{
    return CCommon::AsciiToUnicode(m_version_str);
}

int PluginVersion::GetSubVersion(size_t index) const
{
    if (index < m_version.size())
        return m_version[index];
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPluginUpdateHelper::CheckForUpdate()
{
    m_latest_versions.clear();
    std::wstring url;
    //Update source is Gitee
    if (theApp.m_general_data.update_source == 1)
        url = L"https://gitee.com/zhongyang219/TrafficMonitorPlugins/raw/main/plugins_version.xml";
    //Update source is Github
    else
        url = L"https://raw.githubusercontent.com/zhongyang219/TrafficMonitorPlugins/main/plugins_version.xml";

    //Get url content
    std::string version_info;
    if (CCommon::GetURL(url, version_info))
    {
        //Parse xml and obtain version information
        tinyxml2::XMLDocument doc;
        doc.Parse(version_info.c_str(), version_info.size());
        //Traverse plugin versions
        CTinyXml2Helper::IterateChildNode(doc.RootElement(), [&](tinyxml2::XMLElement* ele) {
            std::wstring file_name = CCommon::StrToUnicode(CTinyXml2Helper::ElementAttribute(ele, "file_name"));
            std::string plugin_version(CTinyXml2Helper::ElementAttribute(ele, "version"));
            PluginVersion version(plugin_version);
            m_latest_versions.emplace(file_name, version);
        });
        return true;
    }
    return false;
}

const PluginVersion& CPluginUpdateHelper::GetPluginLatestVersions(const std::wstring& file_name) const
{
    auto iter = m_latest_versions.find(file_name);
    if (iter != m_latest_versions.end())
    {
        return iter->second;
    }
    static PluginVersion empty_version;
    return empty_version;
}
