#include "stdafx.h"
#include "SkinManager.h"
#include "Common.h"
#include "TrafficMonitor.h"
#include "SettingsHelper.h"

CSkinManager CSkinManager::m_instance;

const std::wstring SKIN_SETTINGS_PREFIX = L"skin_";

void CSkinManager::Init()
{
    if (!m_init)
    {
        m_skins.push_back(L"");
        m_init = true;
    }
}

CSkinManager& CSkinManager::Instance()
{
    return m_instance;
}

std::wstring CSkinManager::GetSkinName(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_skins.size()))
        return m_skins[index];
    else
        return std::wstring();
}

int CSkinManager::FindSkinIndex(const std::wstring& skin_name) const
{
    std::wstring _skin_name{ skin_name };
    SkinNameNormalize(_skin_name);
    for (size_t i{}; i < m_skins.size(); i++)
    {
        if (m_skins[i] == _skin_name)
            return static_cast<int>(i);
    }
    return 0;
}

bool CSkinManager::GetSkinSettingDataByIndex(int index, SkinSettingData& skin_data)
{
    std::wstring skin_name = GetSkinName(index);
    if (!skin_name.empty())
    {
        auto iter = m_skin_setting_data_map.find(skin_name);
        if (iter != m_skin_setting_data_map.end())
        {
            skin_data = iter->second;
            return true;
        }
    }
    return false;
}

const vector<wstring>& CSkinManager::GetSkinNames() const
{
    return m_skins;
}

int CSkinManager::Size() const
{
    return static_cast<int>(m_skins.size());
}

void CSkinManager::AddSkinSettingData(const std::wstring& skin_name, const SkinSettingData& data)
{
    m_skin_setting_data_map[skin_name] = data;
}

void CSkinManager::Save()
{
    //Save independent configurations for all skins
    CSettingsHelper ini;
    for (const auto& data : m_skin_setting_data_map)
    {
        std::wstring app_name = SKIN_SETTINGS_PREFIX + data.first;
        //text color
        ini.SaveMainWndColors(app_name.c_str(), L"text_color", data.second.text_colors);
        ini.WriteBool(app_name.c_str(), L"specify_each_item_color", data.second.specify_each_item_color);
        //font
        ini.SaveFontData(app_name.c_str(), data.second.font);
        //display text
        ini.SaveDisplayStr(app_name.c_str(), data.second.disp_str);
        ini.SavePluginDisplayStr(app_name.c_str(), data.second.disp_str);
    }

    //Remove non-existent skin configuration
    //Get all skin configuration sections
    auto all_skin_section = ini.GetAllAppName(SKIN_SETTINGS_PREFIX);
    for (const auto& skin_name : all_skin_section)
    {
        //Determine whether the skin configuration exists
        auto iter = m_skin_setting_data_map.find(skin_name);
        //If the skin configuration does not exist, remove it from the ini file
        if (iter == m_skin_setting_data_map.end())
        {
            ini.RemoveSection(SKIN_SETTINGS_PREFIX + skin_name);
        }
    }

    ini.Save();
}

void CSkinManager::SkinSettingDataFronSkin(SkinSettingData& skin_setting_data, const CSkinFile& skin_file)
{
    //Get all display items of skin
    std::set<CommonDisplayItem> skin_all_items;
    skin_file.GetSkinDisplayItems(skin_all_items);
    //Get the text color of the skin
    skin_setting_data.specify_each_item_color = skin_file.GetSkinInfo().specify_each_item_color;
    int i{};

    for (const auto& item : skin_all_items)
    {
        skin_setting_data.text_colors[item] = skin_file.GetSkinInfo().TextColor(i);
        i++;
    }
    //Get the font of the skin
    if (!skin_file.GetSkinInfo().font_info.name.IsEmpty())
    {
        skin_setting_data.font = skin_file.GetSkinInfo().font_info;
    }
    else
    {
        skin_setting_data.font.name = theApp.m_str_table.GetLanguageInfo().default_font_name.c_str();
        skin_setting_data.font.size = 10;
    }
    if (skin_file.GetSkinInfo().font_info.size >= MIN_FONT_SIZE && skin_file.GetSkinInfo().font_info.size <= MAX_FONT_SIZE)
        skin_setting_data.font.size = skin_file.GetSkinInfo().font_info.size;

    //Get the display text of the item
    if (!skin_file.GetLayoutInfo().no_label)
    {
        if (!skin_file.GetSkinInfo().display_text.IsInvalid())
        {
            for (const auto& display_item : skin_all_items)
                skin_setting_data.disp_str.Get(display_item) = skin_file.GetSkinInfo().display_text.GetConst(display_item);
        }
        //Get the default display text of the skin
        else
        {
            //Get the default display text for all display items
            for (const auto& display_item : skin_all_items)
                skin_setting_data.disp_str.Get(display_item) = display_item.DefaultString(true);
        }
    }
}

void CSkinManager::SkinNameNormalize(std::wstring& skin_name)
{
    //If there is a slash in front of the name, remove it
    if (!skin_name.empty() && (skin_name[0] == L'\\' || skin_name[0] == L'/'))
    {
        skin_name = skin_name.substr(1);
    }

}
