#pragma once

class PluginVersion
{
public:
    PluginVersion();
    PluginVersion(const std::wstring& version);
    PluginVersion(const std::string& version);

    bool operator<(const PluginVersion& another) const;
    bool operator==(const PluginVersion& another) const;

    std::string GetVersionString() const;
    std::wstring GetVersionWString() const;

protected:
    int GetSubVersion(size_t index) const;

private:
    std::vector<int> m_version;
    std::string m_version_str;
};

class CPluginUpdateHelper
{
public:
    bool CheckForUpdate();

    //Get the latest version of a plugin
    const PluginVersion& GetPluginLatestVersions(const std::wstring& file_name) const;

private:
    std::map<std::wstring, PluginVersion> m_latest_versions;    //The latest version information for each plug-in available online
};

