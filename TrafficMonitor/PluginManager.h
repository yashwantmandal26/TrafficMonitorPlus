#pragma once
#include "PluginInterface.h"
#include <map>
#include "TaskbarItemOrderHelper.h"
#include <functional>

typedef ITMPlugin* (*pfTMPluginGetInstance)();

//Used to load and manage plugins
class CPluginManager
{
public:
    //Plug-in status
    enum class PluginState
    {
        PS_SUCCEED,             //Loading successfully
        PS_MUDULE_LOAD_FAILED,  //dll loading failed
        PS_FUNCTION_GET_FAILED, //Plug-in function acquisition failed
        PS_VERSION_NOT_SUPPORT, //Plugin version is not supported
        PS_DISABLE              //Disabled
    };

    //Plug-in information
    struct PluginInfo
    {
        wstring file_path;      //file path
        HMODULE plugin_module{};  //dll module
        ITMPlugin* plugin{};      //plugin object
        std::vector<IPluginItem*> plugin_items; //All display items provided by the plugin
        PluginState state{};    //Plug-in status
        DWORD error_code{};     //Error code (return value of GetLastError)
        std::map<ITMPlugin::PluginInfoIndex, std::wstring> properties;    //Plugin properties
        std::wstring Property(ITMPlugin::PluginInfoIndex) const;
    };

    CPluginManager();
    ~CPluginManager();
    void LoadPlugins();

    const std::vector<IPluginItem*>& GetPluginItems() const;
    const std::vector<PluginInfo>& GetPlugins() const;
    IPluginItem* GetItemById(const std::wstring& item_id);
    IPluginItem* GetItemByIndex(int index);
    int GetItemIndex(IPluginItem* item) const;
    ITMPlugin* GetPluginByItem(IPluginItem* pItem);
    int GetPluginIndex(ITMPlugin* plugin) const;

    //Iterate through all plugins
    //func: The parameter is the traversed ITMPlugin object
    void EnumPlugin(std::function<void(ITMPlugin*)> func) const;

    //Traverse all plug-in projects
    //func: The parameter is the traversed IPluginItem object
    void EnumPluginItem(std::function<void(IPluginItem*)> func) const;

    const std::set<CommonDisplayItem>& AllDisplayItemsWithPlugins();

    int GetItemWidth(IPluginItem* pItem, CDC* pDC);

private:
    static void ReplacePluginDrawTextFunction(HMODULE plgin_dll_module) noexcept;
    static void ReplaceMfcDrawTextFunction() noexcept;

private:
    std::vector<IPluginItem*> m_plugins;
    std::vector<PluginInfo> m_modules;
    std::set<CommonDisplayItem> m_all_display_items_with_plugins;   //All taskbar display items including plug-ins
    std::map<IPluginItem*, ITMPlugin*> m_plguin_item_map;          //Map used to find the corresponding plug-in based on the plug-in project
};
