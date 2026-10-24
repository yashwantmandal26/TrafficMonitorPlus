#pragma once
#include "PluginInterface.h"

//Built-in display items
enum DisplayItem
{
    TDI_UP,
    TDI_DOWN,
    TDI_CPU,
    TDI_MEMORY,
    TDI_GPU_USAGE,
    TDI_CPU_TEMP,
    TDI_GPU_TEMP,
    TDI_HDD_TEMP,
    TDI_MAIN_BOARD_TEMP,
    TDI_HDD_USAGE,
    TDI_TOTAL_SPEED,
    TDI_CPU_FREQ,
    TDI_TODAY_TRAFFIC
};

//Collection of all built-in display items
const std::set<DisplayItem> AllDisplayItems
{
    TDI_UP, TDI_DOWN, TDI_CPU, TDI_MEMORY, TDI_GPU_USAGE
#ifndef WITHOUT_TEMPERATURE
    , TDI_CPU_TEMP, TDI_GPU_TEMP, TDI_HDD_TEMP, TDI_MAIN_BOARD_TEMP
#endif
    , TDI_HDD_USAGE, TDI_CPU_FREQ, TDI_TOTAL_SPEED, TDI_TODAY_TRAFFIC
};

//Displayed items
class CommonDisplayItem
{
public:
    CommonDisplayItem() {}
    CommonDisplayItem(DisplayItem item);
    CommonDisplayItem(IPluginItem* item);

    bool operator<(const CommonDisplayItem&) const;
    bool operator==(const CommonDisplayItem&) const;

    //Whether it is a plug-in project
    bool IsPlugin() const;

    //Get built-in display items
    DisplayItem ItemType() const;

    //Get plug-in display items
    IPluginItem* PluginItem() const;

    //Get the name of the displayed item
    CString GetItemName() const;

    //Get the default display text of a display item
    std::wstring DefaultString(bool is_main_window) const;

    //Get the name of the key that the display text of a display item is saved in the ini file
    const wchar_t* GetItemIniKeyName() const;

    /**
     * @brief Gets a numerical text that displays the item
     * @param is_main_window If true, it is the main window, otherwise it is the taskbar window
     * @return displayed text
     */
    CString GetItemValueText(bool is_main_window) const;

    /**
     * @brief Gets a numerical example text that displays the item
     * @param is_main_window If true, it is the main window, otherwise it is the taskbar window
     * @return sample text
     */
    CString GetItemValueSampleText(bool is_main_window) const;

    //Whether to occupy two rows exclusively in the taskbar
    bool IsDoubleLineExclusive() const;

private:
    bool is_plugin{};           //Whether it is a plug-in project
    DisplayItem item_type{};    //Built-in display items
    IPluginItem* plugin_item{}; //Plug-in display items
};


class DisplayItemSet
{
public:
    DisplayItemSet(){}
    DisplayItemSet(std::initializer_list<DisplayItem> items);

    void Add(DisplayItem item);
    void Remove(DisplayItem item);
    bool Contains(DisplayItem item) const;

    int ToInt() const;
    void FromInt(int value);
    bool IsEmpty() const;

private:
    std::set<DisplayItem> data;
};
