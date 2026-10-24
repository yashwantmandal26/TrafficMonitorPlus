//This file contains the definition of global structures and enumeration types
#pragma once
#include "stdafx.h"
#include "TaskbarItemOrderHelper.h"

struct Date
{
    int year{};
    int month{};
    int day{};

    int week() const;       //The date is the week of the year

    //Compares the dates of two HistoryTraffic objects and returns true if the time of a is greater than b
    static bool DateGreater(const Date& a, const Date& b);

    //Determine whether the dates of two HistoryTraffic objects are equal
    static bool DateEqual(const Date& a, const Date& b);

};

//Store historical traffic for a certain day
struct HistoryTraffic : public Date
{
    //Traffic used on the day (in KB)
    unsigned __int64 up_kBytes{};
    unsigned __int64 down_kBytes{};
    bool mixed{ true };     //True if no distinction is made between upload and download traffic

    unsigned __int64 kBytes() const;

};

//Colors used in historical traffic statistics to indicate different ranges of traffic
#define TRAFFIC_COLOR_BLUE RGB(0, 183, 238)
#define TRAFFIC_COLOR_GREEN RGB(128, 194, 105)
#define TRAFFIC_COLOR_YELLOE RGB(255, 216, 58)
#define TRAFFIC_COLOR_RED RGB(255, 95, 74)
#define TRAFFIC_COLOR_DARK_RED RGB(166, 19, 0)

//Internet speed unit
enum class SpeedUnit
{
    AUTO,       //automatic
    KBPS,       //KB/s
    MBPS        //MB/s
};


//Hardware monitoring projects
enum HardwareItem
{
    HI_CPU = 1 << 0,        //CPU
    HI_GPU = 1 << 1,        //graphics card
    HI_HDD = 1 << 2,        //hard drive
    HI_MBD = 1 << 3         //motherboard
};

#define DEF_CH L'\"'        //Characters added before and after the string when writing and reading ini file strings
struct DispStrings      //displayed text
{
private:
    std::map<CommonDisplayItem, wstring> map_str;

public:
    //Get a displayed text
    wstring& Get(CommonDisplayItem item);
    const wstring& GetConst(CommonDisplayItem item) const;

    const std::map<CommonDisplayItem, wstring>& GetAllItems() const;

    bool operator==(const DispStrings& disp_str) const;

    //Load the display text of a plug-in project
    void Load(const std::wstring& plugin_id, const std::wstring& disp_str);

    //Is it invalid?
    bool IsInvalid() const;
};

//The action of double-clicking the window with the mouse
enum class DoubleClickAction
{
    CONNECTION_INFO,    //Connection details
    HISTORY_TRAFFIC,    //Historical traffic statistics
    SHOW_MORE_INFO,     //Show more information
    OPTIONS,            //Option settings
    TASK_MANAGER,       //task manager
    SEPCIFIC_APP,       //Specify application
    CHANGE_SKIN,        //Change skin
    NONE                //Take no action
};

//color mode
enum class ColorMode
{
    Default, //Default color
    Light   //light color
};

//Convert the font size to lfHeight in the LOGFONT structure
inline int FontSizeToLfHeight(int font_size, int dpi = 0)
{
    if (dpi == 0)
    {
        HDC hDC = ::GetDC(HWND_DESKTOP);
        dpi = GetDeviceCaps(hDC, LOGPIXELSY);
        ::ReleaseDC(HWND_DESKTOP, hDC);
    }
    int lfHeight = -MulDiv(font_size, dpi, 72);
    return lfHeight;
}

//font
struct FontInfo
{
    CString name;       //Font name
    int size{ 9 };      //font size
    bool bold{};        //Bold
    bool italic{};      //italics
    bool underline{};   //Underline
    bool strike_out{};  //strikethrough

    bool operator==(const FontInfo& a) const;

    //Create a CFont object
    void Create(CFont& font, int dpi = 0) const
    {
        font.CreateFont(
            FontSizeToLfHeight(size, dpi), // nHeight
            0, // nWidth
            0, // nEscapement
            0, // nOrientation
            (bold ? FW_BOLD : FW_NORMAL), // nWeight
            italic, // bItalic
            underline, // bUnderline
            strike_out, // cStrikeOut
            DEFAULT_CHARSET, // nCharSet
            OUT_DEFAULT_PRECIS, // nOutPrecision
            CLIP_DEFAULT_PRECIS, // nClipPrecision
            DEFAULT_QUALITY, // nQuality
            DEFAULT_PITCH | FF_SWISS, // nPitchAndFamily
            name);
    }
};

//Display mode in historical traffic statistics list view
enum class HistoryTrafficViewType
{
    HV_DAY,         //day view
    HV_WEEK,        //Weekly view
    HV_MONTH,          //month view
    HV_QUARTER,     //quarter view
    HV_YEAR            //year view
};

struct StringSet
{
public:
    bool Contains(const std::wstring& str) const;
    void SetStrContained(const std::wstring& str, bool contained);
    void FromString(const std::wstring& str);
    std::wstring ToString() const;
    void FromVector(const std::vector<std::wstring>& vec);
    std::vector<std::wstring> ToVector() const;
    std::set<std::wstring>& data();
private:
    std::set<std::wstring> string_set;
};

struct LanguageInfo
{
    wstring display_name;           // The string displayed in the language settings drop-down menu
    wstring bcp_47;                 // BCP-47 code
    wstring default_font_name;      // Default font
    wstring translator;
    wstring translator_url;
    WORD language_id{};

    bool operator==(const LanguageInfo& another) const;
    wstring toConfigString() const;
    void fromConfigString(const wstring& config_str);
    bool isEmpty() const;
};

//option setting data
struct MainConfigData
{
    int m_transparency{ 100 };          //window transparency
    bool m_show_more_info{ false };     //Show more information
    bool m_show_task_bar_wnd{ false };  //Show taskbar window
    bool m_hide_main_window;            //Hide main window
    //bool m_tbar_show_cpu_memory; //The taskbar window displays CPU and memory utilization

    int m_position_x;   //x coordinate of window position
    int m_position_y;   //The y coordinate of the window position

    bool m_auto_select{ false };    //Automatically select connection
    bool m_select_all{ false };     //Count the speed of all connections
    string m_connection_name;      //The name of the currently selected network

    wstring m_skin_name;            //The name of the selected skin

    bool skin_auto_adapt{ false };  //Automatically switch skin based on dark/light mode
    wstring skin_name_light_mode;   //Skin name in light mode
    wstring skin_name_dark_mode;    //Skin name in dark mode

    int m_dft_notify_icon = 0;      //Default notification icon (used to distinguish between dark and light modes of win10)
    int m_notify_icon_selected{};   //Notification area icon to display
    bool m_notify_icon_auto_adapt{ false }; //Whether the notification area icon automatically adapts to the dark and light color modes of Win10

    //bool m_show_internet_ip{ false }; //Whether to display the external IP address in the "Connection Details" dialog box
    bool m_use_log_scale{ false };          //Whether to use a logarithmic scale when drawing a rectangle representing historical traffic values in the "Historical Traffic Statistics" dialog box
    HistoryTrafficViewType m_view_type{};
    bool m_sunday_first{ true };            //Whether to use Sunday as the first day of the week
    StringSet plugin_disabled;      //Disabled plugins

};

//Memory display mode
enum class MemoryDisplay
{
    USAGE_PERCENTAGE,       //Percentage used
    MEMORY_USED,            //memory used
    MEMORY_AVAILABLE        //memory available
};

//Data saved separately for each skin
struct SkinSettingData
{
    FontInfo font;          //font
    DispStrings disp_str;   //displayed text
    std::map<CommonDisplayItem, COLORREF> text_colors{};    //text color
    bool specify_each_item_color{};

    bool IsEmpty() const;
    bool operator==(const SkinSettingData& a) const;
};

//Common data in "Main Window Settings" and "Taskbar Window Settings" in option settings (this structure is not used to create objects)
struct PublicSettingData
{
    bool specify_each_item_color{ false };      //Whether to specify the color of each item
    FontInfo font;          //font
    DispStrings disp_str;   //displayed text
    bool speed_short_mode{ false };     //Simple mode for network speed display (reduce the number of decimal places and do not display “B” in units)
    bool separate_value_unit_with_space{ true };    //Network speed value and unit are separated by spaces.
    bool show_tool_tip{ true };         //Show mouse tip
    MemoryDisplay memory_display{ MemoryDisplay::USAGE_PERCENTAGE };    //Memory display mode

    bool unit_byte{ true };             //Use bytes (B) instead of bits (b) as units
    SpeedUnit speed_unit;       //unit of internet speed
    bool hide_unit;         //Hidden units
    bool hide_percent;      //Hide percent sign
    DoubleClickAction double_click_action;      //Mouse double click action
    wstring double_click_exe;   //When the mouse double-click action is to open the specified application, the opened program path
};

//#define MAIN_WND_COLOR_NUM 9 //Number of main window colors
//Data of "Main Window Settings" in option settings
struct MainWndSettingData : public PublicSettingData
{
    std::map<CommonDisplayItem, COLORREF> text_colors{};    //text color
    bool swap_up_down{ false };     //Swap upload and download display locations
    bool hide_main_wnd_when_fullscreen;     //Hide the floating window when a program is running in full screen
    bool m_always_on_top{ false };      //Window on top
    bool m_lock_window_pos{ false };    //Lock window position
    bool m_mouse_penetrate{ false };    //mouse penetration
    bool m_alow_out_of_border{ false };     //Whether to allow floating windows to exceed the boundaries of the screen

    void FormSkinSettingData(const SkinSettingData& sking_setting_data);
    SkinSettingData ToSkinSettingData() const;
};

//#define TASKBAR_COLOR_NUM 18 //Number of taskbar window colors

struct TaskbarItemColor //The color of each item in the taskbar window
{
    COLORREF label{};   //Label color
    COLORREF value{};   //numerical color

    bool operator==(const TaskbarItemColor& item) const
    {
        return label == item.label && value == item.value;
    }
};

//Data for "Taskbar Window Settings" in Options Settings
struct TaskBarSettingData : public PublicSettingData
{
    COLORREF  back_color{ RGB(0, 0, 0) };   //background color
    COLORREF transparent_color{ RGB(0, 0, 0) };     //Transparent color
    COLORREF status_bar_color{ RGB(0, 0, 0) };      // CPU/memory status bar color
    std::map<CommonDisplayItem, TaskbarItemColor> text_colors{};    //text color
    int dft_back_color = 0;                         //Default background color
    int dft_transparent_color = 0;                  //Default transparent color
    int dft_status_bar_color = 0x005A5A5A;          //Default CPU/memory status bar color
    int dft_text_colors = 0x00ffffffU;              //Default text color

    bool auto_adapt_light_theme{ true };            //Whether to automatically adapt to light themes
    int dark_default_style{ 0 };                    //The default scheme used when using dark themes
    int light_default_style{ -1 };                  //The default scheme used when using light themes
    bool auto_set_background_color{ false };        //Automatically set background color based on taskbar color
    bool auto_save_taskbar_color_settings_to_preset{};    //When "Automatically adapt to Windows 10 dark/light theme" is enabled, whether to automatically save the current color settings to the corresponding preset when the color settings are changed
    bool IsTaskbarTransparent() const;
    void SetTaskabrTransparent(bool transparent);

    CTaskbarItemOrderHelper item_order;
    DisplayItemSet display_item{ TDI_UP, TDI_DOWN };      //Items displayed in the taskbar window
    StringSet plugin_display_item;                  //Plug-in projects displayed in the task window

    bool show_taskbar_wnd_in_secondary_display{ false };    //Whether to display the taskbar window on the secondary monitor
    int secondary_display_index{};      //Display the taskbar window on which secondary monitor
    bool value_right_align{ false };    //Whether the value is right aligned
    int digits_number{ 4 };             //Number of data bits
    bool horizontal_arrange{ true };    //Arrange horizontally
    bool show_status_bar{ true };       //Display CPU/memory status bar
    bool tbar_wnd_on_left{ false };     //If true, the taskbar window is displayed to the left of (or above) the taskbar
    bool tbar_wnd_snap{ false };     	//If true, the taskbar window snaps to the middle taskbar in Win11, otherwise it is close to the edge
    bool cm_graph_type{ false };        //If it is false, the default style is the original style, and the histogram displays the occupancy rate. If it is true, the occupancy rate is displayed by scrolling.
    bool show_graph_dashed_box{ true }; //Show occupancy map dashed box

    int item_space{};                   //Item spacing
    int vertical_margin{};              //Item vertical spacing
    int window_offset_top{};            //Taskbar window top margin
    int window_offset_left{};           //Taskbar window left margin
    void ValidItemSpace();
    void ValidVerticalMargin();
    void ValidWindowOffsetTop();
    void ValidWindowOffsetLeft();
    bool avoid_overlap_with_widgets{ false };   //Avoid overlapping with right widgets
    int taskbar_left_space_win11{};         //Under Windows 11, the width of taskbar gadgets
    int taskbar_right_space_win11{};        //Under Windows 11, the width of the taskbar window from the right side of the taskbar (valid only when the position of the taskbar TrayNotifyWnd window cannot be obtained)

    bool show_netspeed_figure{ false };     //Whether to display the network speed usage graph
    int netspeed_figure_max_value;          //Maximum value of network speed usage chart
    int netspeed_figure_max_value_unit{};   //The unit of the maximum value of the network speed occupancy graph (0: KB, 1: MB)
    unsigned __int64 GetNetspeedFigureMaxValueInBytes() const;  //Get the maximum value of the network speed usage map (in bytes)
    bool graph_color_following_system{ false }; //The color of the occupancy map follows the system theme color
    COLORREF GetUsageGraphColor() const;    //Get the color of the occupancy map

    bool disable_d2d{ false };//Whether to disable d2d drawing
    DWORD update_layered_window_error_code{0}; // When using D2D1 rendering, the error code of UpdateLayeredWindowIndirect failure will be reset to 0 when closing the taskbar window.
    bool enable_colorful_emoji{ true };       //Whether to display colored emojis

    bool is_windows_web_experience_detected{ false }; //Whether Windows Web Experience widget installation information is detected
};

//Data for "General Settings" in Options Settings
struct GeneralSettingData
{
    bool check_update_when_start{ true };
    int update_source{};                    //Update source. 0: GitHub; 1: Gite
    bool auto_run{ false };
    bool auto_run_by_task_scheduler{ false };   //Whether to enable self-starting on boot through task scheduler
    bool show_notify_icon{ true };    //Show notification area icon
//notification message
    bool traffic_tip_enable{ false };       //Whether to enable prompts when traffic exceeds
    int traffic_tip_value;                  //Traffic threshold to be prompted
    int traffic_tip_unit{};                 //The unit of the traffic value to be prompted (0: MB, 1: GB)

    struct NotifyTipSettings        //Settings for prompts to pop up when a certain value is exceeded
    {
        bool enable;                //Whether to enable prompts
        int tip_value;              //The critical value to pop up a prompt
    };
    NotifyTipSettings memory_usage_tip;     //Memory usage exceeded prompt
    NotifyTipSettings cpu_temp_tip;         //CPU temperature exceeded prompt
    NotifyTipSettings gpu_temp_tip;         //Graphics card temperature exceeds prompt
    NotifyTipSettings hdd_temp_tip;         //Hard drive temperature exceeds prompt
    NotifyTipSettings mainboard_temp_tip;   //Motherboard temperature exceeds prompt


    //language
    LanguageInfo language;

    bool show_all_interface{ true };

    //How to obtain CPU utilization
    enum CpuUsageAcquireMethod
    {
        CA_CPU_TIME,    //usage time
        CA_PDH         //Performance counters
    };
    CpuUsageAcquireMethod cpu_usage_acquire_method{};  //How to get CPU utilization

    bool portable_mode{ false };        //Portable mode, if true, all data of the program will be saved to the directory where the exe is located, otherwise it will be saved to the Appdata\Romaing directory.
    int monitor_time_span{ 1000 };    //Monitoring interval

    std::wstring hard_disk_name;        //The name of the hard disk to be monitored
    std::wstring cpu_core_name;         //The name of the CPU core to monitor

    unsigned int hardware_monitor_item{};   //What hardware to monitor
    bool IsHardwareEnable(HardwareItem item_type) const
    {
        return hardware_monitor_item & item_type;
    }
    void SetHardwareEnable(HardwareItem item_type, bool enable)
    {
        if (enable)
            hardware_monitor_item |= item_type;
        else
            hardware_monitor_item &= ~item_type;
    }

    StringSet connections_hide;     //Used to save which networks to hide from the "Select Network Connections" submenu item
};

//Define the maximum and minimum values for which the monitoring time interval is valid
#define MONITOR_TIME_SPAN_MIN 200
#define MONITOR_TIME_SPAN_MAX 30000

//Pass a reference to a bool variable through the constructor, set it to true during construction, and set it to false during destruction.
class CFlagLocker
{
public:
    CFlagLocker(bool& flag)
        : m_flag(flag)
    {
        m_flag = true;
    }

    ~CFlagLocker()
    {
        m_flag = false;
    }

private:
    bool& m_flag;
};
