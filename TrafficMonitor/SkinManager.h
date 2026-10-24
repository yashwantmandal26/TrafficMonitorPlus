#pragma once
#include "CommonData.h"
#include "SkinFile.h"

class CSkinManager
{
public:
    void Init();
    static CSkinManager& Instance();
    std::wstring GetSkinName(int index) const;
    int FindSkinIndex(const std::wstring& skin_name) const;
    bool GetSkinSettingDataByIndex(int index, SkinSettingData& skin_data);
    const vector<wstring>& GetSkinNames() const;
    int Size() const;
    void AddSkinSettingData(const std::wstring& skin_name, const SkinSettingData& data);

    void Save();

    //Get the SkinSettingData object from a skin file
    static void SkinSettingDataFronSkin(SkinSettingData& skin_setting_data, const CSkinFile& skin_file);

    //Remove the slash in front of the skin name
    static void SkinNameNormalize(std::wstring& skin_name);

private:
    CSkinManager()
    {}

private:
    vector<wstring> m_skins;    //Path to store skin files
    std::map<std::wstring, SkinSettingData> m_skin_setting_data_map;    //Save font and display text settings for each skin

    static CSkinManager m_instance;
    bool m_init{ false };
};

