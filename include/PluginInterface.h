/*********************************************************
* TrafficMonitor plug-in interface
* Copyright (C) by Zhong Yang 2021
* zhongyang219@hotmail.com
**********************************************************/
#pragma once

class IPluginDrawer;

//The plug-in displays the interface of the project
class IPluginItem
{
public:
    /**
     * @brief Get the name of the displayed item
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetItemName() const = 0;

    /**
     * @brief Get the unique ID of the displayed item
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetItemId() const = 0;

    /**
     * @brief Get the text of the project label
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetItemLableText() const = 0;

    /**
     * @brief Get the text of the item value
     * @detail Since this function may be called frequently, do not obtain monitoring data here.
     *  Instead, obtain the data in the ITMPlugin::DataRequired function, save it, and then return the obtained value here.
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetItemValueText() const = 0;

    /**
     * @brief Get the sample text of the item value
     * @detail The length of the string returned by this function will be used to calculate the width of the display area
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetItemValueSampleText() const = 0;

    /**
     * @brief Whether the display area is drawn by the plug-in itself
     * @detail
     *  If false is returned, the display area is drawn by the main program based on the text returned by GetItemLableText and GetItemValueText, and overriding the DrawItem function will not work.
     *  If you override this function and return true, you must override the DrawItem function and add the code to draw the display area inside,
     *  At this time, the return values of GetItemLableText, GetItemValueText and GetItemValueSampleText will be ignored by the main program.
     * @return  bool
     */
    virtual bool IsCustomDraw() const { return false; }

    /**
     * @brief Get the width of the display area
     * @detail
     *  Overriding this function will only work if the CustomDraw() function returns true.
     *  The returned value is the width when the DPI is 96 (100%). The main program will automatically scale up according to the current system DPI setting.
     *  So you don't need to return different values for different DPI settings.
     *  It should be noted that the return value here represents the minimum width required for the self-drawn area. The value of the parameter w in the DrawItem function may be greater than this value.
     * @return  int
     */
    virtual int GetItemWidth() const { return 0; }

    /**
     * @brief Customize the function for drawing the display area. Rewriting this function is only effective when the CustomDraw() function returns true.
     * @param void * hDC drawing context handle
     * @param int x rectangular area of drawing
     * @param   int y
     * @param   int w
     * @param   int h
     * @param bool dark_mode dark mode is true, light mode is false
     * @return  void
     */
    virtual void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) {}

    /**
     * @brief Get the width of the display area
     * @detail
     *  Overriding this function will only work if the CustomDraw() function returns true.
     *  This function is different from GetItemWidth. The plug-in can calculate the required width based on the parameter hDC.
     *  It returns the actual width, and the main program will not enlarge the return value according to the DPI of the current system.
     *  It should be noted that the return value here represents the minimum width required for the self-drawn area. The value of the parameter w in the DrawItem function may be greater than this value.
     * @param void * hDC drawing context handle
     * @return  int
     */
    virtual int GetItemWidthEx(void* hDC) const { return 0; }

    /** Type of mouse event */
    enum MouseEventType
    {
        MT_LCLICKED,    /**< Clicked the left mouse button */
        MT_RCLICKED,    /**< Clicked the right mouse button */
        MT_DBCLICKED,   /**< Double-clicked the left mouse button */
        MT_WHEEL_UP,    /**< Mouse wheel scrolled up */
        MT_WHEEL_DOWN,  /**< Mouse wheel scrolled down */
    };

    enum MouseEventFlag
    {
        MF_TASKBAR_WND = 1 << 0,        /**< Whether it is a mouse event of the taskbar window */
    };

    /**
     * @brief Called by the main program when there is a mouse event in the plug-in display area
     * @param MouseEventType type type of mouse event
     * @param int x x coordinate of the mouse pointer
     * @param int y The y coordinate of the mouse pointer
     * @param void* hWnd The handle of the window that generated this mouse event (main window or taskbar window)
     * @param int flag is a combination of several MouseEventFlag enumeration constants
     * @return int If non-0 is returned, the main program believes that the plug-in has fully responded to this mouse event, and the main program will not make additional responses to this mouse event.
     *   For example, when type is MT_RCLICKED, if the program returns 0, the right-click menu provided by the main program will pop up; if it returns non-0, the main program will not do any processing.
     */
    virtual int OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag) { return 0; }

    enum KeyboardEventFlag
    {
        KF_TASKBAR_WND = 1 << 0,        /**< Whether it is a keyboard event of the taskbar window */
    };

    /**
     * @brief Called by the main program when there is a keyboard event in the plug-in display area
     * @param int key pressed key
     * @param bool ctrl Whether the Ctrl key is pressed
     * @param bool shift whether the Shift key is pressed
     * @param bool alt Whether the Alt key is pressed
     * @param void* hWnd The handle of the window that generated this keyboard event (main window or taskbar window)
     * @param int flag is a combination of several KeyboardEventFlag enumeration constants
     * @return int If non-0 is returned, the main program believes that the plug-in has fully responded to this keyboard event, and the main program will not make additional responses to this keyboard event.
     */
    virtual int OnKeboardEvent(int key, bool ctrl, bool shift, bool alt, void* hWnd, int flag) { return 0; }

    enum ItemInfoType
    {

    };
    //Reserved interface
    virtual void* OnItemInfo(ItemInfoType, void* para1, void* para2) { return 0; }

    /**
     * @brief Whether to display the resource usage graph of this project in the taskbar
     * @return 1: display, 0: not display
     */
    virtual int IsDrawResourceUsageGraph() const { return 0; }

    /**
     * @brief Gets the value of the resource usage map. Valid when the return value of IsDrawResourceUsageGraphType is not 0
     * @return float The value of the resource usage map, ranging from 0.0 to 1.0.
     */
    virtual float GetResourceUsageGraphValue() const { return 0.0; }

    /**
     * @brief Customize the function for drawing the display area. Rewriting this function is only effective when the CustomDraw() function returns true.
     * @param IPluginDrawer* pDrawer pointer for the plug-in drawing interface
     * @param int x rectangular area of drawing
     * @param   int y
     * @param   int w
     * @param   int h
     * @param bool dark_mode dark mode is true, light mode is false
     * @return  bool
     */
    virtual bool DrawItemEx(IPluginDrawer* pDrawer, int x, int y, int w, int h, bool dark_mode) { return false; }

    /**
     * @brief Whether to exclusive double rows in the task
     * @return 1: yes, 0: no
     */
    virtual int IsDoubleLineExclusive() const { return 0; }
};

class ITrafficMonitor;

///////////////////////////////////////////////////////////////////////////////////////////////////////
//Plug-in interface
class ITMPlugin
{
public:
    /**
     * @brief The version of the plug-in interface. The return value here will be modified only when the plug-in interface is modified.
     * @attention Plug-in developers should not modify the return value here, nor should they override this virtual function.
     * @return  int
     */
    virtual int GetAPIVersion() const { return 8; }

    /**
     * @brief Gets the object of the plug-in display item
     * @detail A plug-in dll can provide multiple objects that implement the IPluginItem interface, corresponding to multiple display items.
     *  When the value of index is greater than or equal to 0 and less than the number of objects of the IPluginItem interface, the pointer of the IPluginItem interface of the object is returned. In other cases, a null pointer should be returned.
     *  For example, if the plug-in provides two display items, when the index is equal to 0 or 1, an object corresponding to the IPluginItem interface is returned. For other values, a null pointer must be returned.
     * @param int index The index of the object
     * @return IPluginItem* The object of the plug-in display item
     */
    virtual IPluginItem* GetItem(int index) = 0;

    /**
     * @brief The main program will call this function at regular intervals, and the plug-in needs to obtain the monitored data in the function.
     */
    virtual void DataRequired() = 0;

    /** Return value of option setting dialog box */
    enum OptionReturn
    {
        OR_OPTION_CHANGED,          /**< The option settings were changed in the option settings dialog box */
        OR_OPTION_UNCHANGED,        /**< The option settings were not changed in the option settings dialog box */
        OR_OPTION_NOT_PROVIDED      /**< No option settings dialog provided */
    };

    /**
     * @brief The main program calls this function to open the plug-in option setting dialog box
     * @detail This function does not necessarily need to be overridden. If the plug-in provides an option setting interface, this function should be overridden and return OR_OPTION_CHANGED or OR_OPTION_UNCHANGED at the end.
     * @param void * hParent The handle of the parent window
     *  If the return value is OR_OPTION_NOT_PRVIDED, it is considered that the plug-in does not provide an option setting dialog box.
     * @return  ITMPlugin::OptionReturn
     */
    virtual OptionReturn ShowOptionsDialog(void* hParent) { return OR_OPTION_NOT_PROVIDED; }

    /** Index of plug-in information */
    enum PluginInfoIndex
    {
        TMI_NAME,           /**< Name */
        TMI_DESCRIPTION,    /**< Description */
        TMI_AUTHOR,         /**< Author */
        TMI_COPYRIGHT,      /**< Copyright */
        TMI_VERSION,        /**< Version */
        TMI_URL,            /**< Home */
        TMI_MAX             /**< Maximum value of plug-in information */
    };

    /**
     * @brief Get the information of this plug-in and return the corresponding information according to the value of index
     */
    virtual const wchar_t* GetInfo(PluginInfoIndex index) = 0;

    /** Monitoring information of the main program */
    struct MonitorInfo
    {
        unsigned long long up_speed{};
        unsigned long long down_speed{};
        int cpu_usage{};
        int memory_usage{};
        int gpu_usage{};
        int hdd_usage{};
        int cpu_temperature{};
        int gpu_temperature{};
        int hdd_temperature{};
        int main_board_temperature{};
        int cpu_freq{};
    };

    /**
     * @brief The main program calls this function to pass all obtained monitoring information to the plug-in
     */
    virtual void OnMonitorInfo(const MonitorInfo& monitor_info) {}

    /**
     * @brief Gets the text to be displayed by the plug-in in the mouse prompt
     */
    virtual const wchar_t* GetTooltipInfo() { return L""; }

    enum ExtendedInfoIndex
    {
        EI_LABEL_TEXT_COLOR,    //Label text color for drawing
        EI_VALUE_TEXT_COLOR,    //Numeric text color for plots
        EI_DRAW_TASKBAR_WND,    //Whether to draw the taskbar window

        //Main window option settings
        EI_NAIN_WND_NET_SPEED_SHORT_MODE,   //Network speed display simple mode
        EI_MAIN_WND_SPERATE_WITH_SPACE,     //Use spaces to separate values and units
        EI_MAIN_WND_UNIT_BYTE,              //Whether the network speed unit uses B (bytes)
        EI_MAIN_WND_UNIT_SELECT,            //Network speed unit selection (0: automatic, 1: fixed to KB/s, 2: fixed to MB/s)
        EI_MAIN_WND_NOT_SHOW_UNIT,          //Do not display network speed units
        EI_MAIN_WND_NOT_SHOW_PERCENT,       //Do not show percent signs

        //Taskbar window settings
        EI_TASKBAR_WND_NET_SPEED_SHORT_MODE,    //Network speed display simple mode
        EI_TASKBAR_WND_SPERATE_WITH_SPACE,      //Use spaces to separate values and units
        EI_TASKBAR_WND_VALUE_RIGHT_ALIGN,       //Right-justify numeric values
        EI_TASKBAR_WND_NET_SPEED_WIDTH,         //Internet speed data width
        EI_TASKBAR_WND_UNIT_BYTE,               //Whether the network speed unit uses B (bytes)
        EI_TASKBAR_WND_UNIT_SELECT,             //Network speed unit selection (0: automatic, 1: fixed to KB/s, 2: fixed to MB/s)
        EI_TASKBAR_WND_NOT_SHOW_UNIT,           //Do not display network speed units
        EI_TASKBAR_WND_NOT_SHOW_PERCENT,        //Do not show percent signs

        EI_CONFIG_DIR,                      //Directory of configuration files
    };

    /**
     * @brief The main program calls this function to pass more information to the plugin
     * @param ExtendedInfoIndex index The index of the information, used to distinguish the information passed to the plug-in
     * @param const wchar_t* data passed data
     * @return  void
     */
    virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) {}

    /**
     * @brief Get the icon of the plug-in, HICON format
     */
    virtual void* GetPluginIcon() { return nullptr; }

    /**
     * @brief Get the number of plugin commands
     * @return int The number of plugin commands
     */
    virtual int GetCommandCount() { return 0; }

    /**
     * @brief Get the command name of the plug-in
     * @param int command_index The index of the plug-in command (starting from 0, less than the return value of GetCommandCount())
     * @return wchar_t* The name of the plug-in command
     */
    virtual const wchar_t* GetCommandName(int command_index) { return nullptr; }

    /**
     * @brief Get the command icon of the plug-in
     * @param int command_index The index of the plug-in command (starting from 0, less than the return value of GetCommandCount())
     * @return void* Icon of the plug-in command, HICON format
     */
    virtual void* GetCommandIcon(int command_index) { return nullptr; }

    /**
     * @brief is called by the framework when executing a plugin command
     * @param int command_index The index of the plug-in command (starting from 0, less than the return value of GetCommandCount())
     * @param void* hWnd The window handle to which this command is sent
     * @param void* para reserved parameters
     */
    virtual void OnPluginCommand(int command_index, void* hWnd, void* para) {}

    /**
    * @brief Gets whether the plug-in command is checked
    * @param int command_index The index of the plug-in command (starting from 0, less than the return value of GetCommandCount())
    * @return 1: checked; 0: not checked
    */
    virtual int IsCommandChecked(int command_index) { return false; }

    /**
     * @brief plugin initialization
     * @detail is called when the plug-in is loaded, passing a pointer to the ITrafficMonitor interface. Plugins can save this pointer to call functions in the ITrafficMonitor interface
     * @param   pApp
     */
    virtual void OnInitialize(ITrafficMonitor* pApp) {}
};


///////////////////////////////////////////////////////////////////////////////////////////////////////
//Main program interface
class ITrafficMonitor
{
public:
    /**
     * @brief Gets the version of this interface.
     * @attention The plug-in needs to first determine the version of the interface when calling the function in ITrafficMonitor.
     * @return  int
     */
    virtual int GetAPIVersion() = 0;

    /**
     * @brief Get the version of TrafficMonitor.
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetVersion() = 0;

    /** All monitoring information of the main program */
    enum MonitorItem
    {
        MI_UP,                  /**< Upload speed */
        MI_DOWN,                /**< Download speed */
        MI_CPU,                 /**< CPU utilization */
        MI_MEMORY,              /**< Memory utilization */
        MI_GPU_USAGE,           /**< Graphics card utilization */
        MI_CPU_TEMP,            /**< CPU temperature */
        MI_GPU_TEMP,            /**< Graphics card temperature */
        MI_HDD_TEMP,            /**< Hard disk temperature */
        MI_MAIN_BOARD_TEMP,     /**< Motherboard temperature */
        MI_HDD_USAGE,           /**< Hard drive utilization */
        MI_CPU_FREQ,            /**< CPU frequency */
        MI_TODAY_UP_TRAFFIC,    /**< Today’s upload traffic */
        MI_TODAY_DOWN_TRAFFIC   /**< Today’s download traffic */
    };

    /**
     * @brief Gets the value of monitoring information of a main program
     *          (ITMPlugin::OnMonitorInfo will be deprecated)
     * @param item The item to obtain monitoring information
     * @return Obtain monitoring information
     */
    virtual double GetMonitorValue(MonitorItem item) = 0;

    /**
     * @brief Gets the value of a main program monitoring information in the form of a string
     * @param item The item to obtain monitoring information
     * @param is_main_window is the value of the main window
     * @return Obtain monitoring information
     */
    virtual const wchar_t* GetMonitorValueString(MonitorItem item, int is_main_window = false) = 0;

    /**
     * @brief displays a notification message
     * @param strMsg Notification message to display
     */
    virtual void ShowNotifyMessage(const wchar_t* strMsg) = 0;

    /**
     * @brief Get the current language id
     * @return the language id of the current main program
     */
    virtual unsigned short GetLanguageId() const = 0;

    /**
     * @brief Get the configuration file directory
     * @return configuration file directory
     */
    virtual const wchar_t* GetPluginConfigDir() const = 0;

    /** Main program DPI type */
    enum DPIType
    {
        DPI_MAIN_WND,   /**< DPI of main window */
        DPI_TASKBAR     /**< DPI of taskbar window */
    };

    /**
     * @brief Get the main program DPI
     * @return main program DPI
     */
    virtual int GetDPI(DPIType type) const = 0;

    /**
     * @brief Get the current system theme color
     * @return color value in COLORREF format
     */
    virtual unsigned int GetThemeColor() const = 0;

    /**
     * @brief Get the string resource of the current language
     * @param str_id string resource id
     * @param str_section The section name of the string resource in the ini file
     * @return 
     */
    virtual const wchar_t* GetStringRes(const wchar_t* key, const wchar_t* section) = 0;

    /**
     * @brief Get the handle of the main window
     * @return  
     */
    virtual void* GetMainWindowHwnd() = 0;

    /**
     * @brief Gets the handle of the taskbar window. When the taskbar window is not displayed, the return value is empty.
     * @return  
     */
    virtual void* GetTaskbarWindowHwnd() = 0;

    /**
     * API Version
     *      1 Added GetMainWindowHwnd and GetTaskbarWindowHwnd functions
     */
};


//Plug-in drawing interface
class IPluginDrawer
{
public:
    /**
     * @brief Gets the version of this interface.
     * @return  int
     */
    virtual int GetAPIVersion() = 0;

    // stretch mode
    enum StretchMode
    {
        STRETCH,    /**< Stretching will change the proportion */
        FILL,       /**< Fill, do not change the proportion, will crop the long edge */
        FIT         /**< Adapt without changing proportions or cropping */
    };

    //Alignment
    enum Alignment
    {
        LEFT,       /**< Align left */
        RIGHT,      /**< Align right */
        CENTER,     /**< Center */
    };

    /**
     * @brief draws text within the specified rectangular area
     * @param x left coordinate of the rectangular area
     * @param y The top coordinate of the rectangular area
     * @param w width of rectangular area
     * @param h the height of the rectangular area
     * @param lpszString drawn text
     * @param color The color of the text, the format is 0xBBGGRR
     * @param align text alignment
     * @param multi_line Whether to display multiple lines
     * @param alpha opacity, 0~255
     */
    virtual void DrawWindowText(int x, int y, int w, int h, const wchar_t* lpszString, unsigned long color, Alignment align = Alignment::LEFT, bool multi_line = false, unsigned char alpha = 255) = 0;

    /**
     * @brief sets the drawing clipping area
     * @param x left coordinate of the rectangular area
     * @param y The top coordinate of the rectangular area
     * @param w width of rectangular area
     * @param h the height of the rectangular area
     */
    virtual void SetDrawRect(int x, int y, int w, int h) = 0;

    /**
     * @brief Fill the rectangle with a solid color
     * @param x left coordinate of the rectangular area
     * @param y The top coordinate of the rectangular area
     * @param w width of rectangular area
     * @param h the height of the rectangular area
     * @param color fill color, format is 0xBBGGRR
     * @param alpha opacity, 0~255
     */
    virtual void FillRect(int x, int y, int w, int h, unsigned long color, unsigned char alpha = 255) = 0;
    
    /**
     * @brief draws a rectangular border
     * @param x left coordinate of the rectangular area
     * @param y The top coordinate of the rectangular area
     * @param w width of rectangular area
     * @param h the height of the rectangular area
     * @param color The color of the border, the format is 0xBBGGRR
     * @param width The line width of the border
     * @param dot_line whether it is a dotted line
     * @param alpha opacity, 0~255
     * @param radius rounded corner radius, if it is 0, it is a right rectangle
     */
    virtual void DrawRectOutLine(int x, int y, int w, int h, unsigned long color, int width = 1, bool dot_line = false, unsigned char alpha = 255, int radius = 0) = 0;

    /**
     * @brief draws a bitmap
     * @param hbitmap bitmap handle
     * @param x left coordinate of the rectangular area
     * @param y The top coordinate of the rectangular area
     * @param w width of rectangular area
     * @param h the height of the rectangular area
     * @param stretch_mode Drawing stretch mode
     * @param alpha opacity, 0~255
     */
    virtual void DrawBitmap(void* hbitmap, int x, int y, int w, int h, StretchMode stretch_mode = StretchMode::STRETCH, unsigned char alpha = 255) = 0;

    /**
     * @brief draw an icon
     * @param hIcon handle of the icon
     * @param x left coordinate of the rectangular area
     * @param y The top coordinate of the rectangular area
     * @param w width of rectangular area
     * @param h the height of the rectangular area
     */
    virtual void DrawIcon(void* hIcon, int x, int y, int w, int h) = 0;

    /**
     * @brief calculates the width and height of a string
     * @param lpszString string
     * @param[out] w width of string
     * @param[out] h The height of the string
     */
    virtual void GetTextExtent(const wchar_t* lpszString, int& w, int& h) = 0;

    /**
     * @brief draws a line segment
     * @param x1 x coordinate of the first point
     * @param y1 y coordinate of the first point
     * @param x2 The x coordinate of the second point
     * @param y2 7 coordinates of the second point
     * @param color color
     * @param alpha opacity
     */
    virtual void DrawLine(int x1, int y1, int x2, int y2, unsigned long color, int width = 1, bool dot_line = false, unsigned char alpha = 255) = 0;

    /**
     * @brief Get the handle of GDI drawing
     * @return HDC format handle
     */
    virtual void* GetHDC() = 0;
};


/*
* Note: The plug-in dll needs to export the following functions
* ITMPlugin* TMPluginGetInstance();
*   The function returns a pointer to an object of the ITMPlugin interface.
*   This object should usually be a global or static object, and it should not be released before the program ends.
*/


/*
* Update record:
* ITMPlugin
* -------------------------------------------------------------------------
* API version | Update content
* -------------------------------------------------------------------------
*     1 | First version
* -------------------------------------------------------------------------
*     2 | Added ITMPlugin::GetTooltipInfo function
* -------------------------------------------------------------------------
*     3 | Added IPluginItem::GetItemWidthEx, IPluginItem::OnMouseEvent functions
* -------------------------------------------------------------------------
*     4 | Added IPluginItem::OnKeboardEvent IPluginItem::OnItemInfo function
* -------------------------------------------------------------------------
*     5 | Added ITMPlugin::GetCommandName ITMPlugin::GetCommandIcon ITMPlugin::OnPluginCommand function
* -------------------------------------------------------------------------
*     6 | Added IPluginItem::GetResourceUsageGraphType IPluginItem::GetResourceUsageGraphValue function
* -------------------------------------------------------------------------
*     7 | Added ITMPlugin::OnInitialize function
* -------------------------------------------------------------------------
*     8 | Added IPluginItem::DrawItemEx, IPluginItem::IsDoubleLineExclusive functions
* -------------------------------------------------------------------------
*/
