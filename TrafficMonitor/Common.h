#pragma once
#include "CommonData.h"
#include "CVariant.h"
#include <set>
#include <functional>

class CCommon
{
public:
    CCommon();
    ~CCommon();
    //Convert const char* string to wide character string
    static wstring StrToUnicode(const char* str, bool utf8 = false);

    static string UnicodeToStr(const wchar_t* wstr, bool utf8 = false);

    static wstring AsciiToUnicode(const string& str);
    static string AsciiToStr(const std::wstring& wstr);

    static void StringNormalize(std::string& str);
    static void StringNormalize(std::wstring& str);

    //Split a string into several characters (the template type can only be string or wstring)
    //str: original string
    //div_ch: character used for division
    //result: receive the split result
    static void StringSplit(const std::string& str, char div_ch, vector<std::string>& results, bool skip_empty = true, bool trim = true);
    static void StringSplit(const std::wstring& str, wchar_t div_ch, vector<std::wstring>& results, bool skip_empty = true, bool trim = true);

    static void StringSplit(const std::string& str, const std::string& div_str, vector<std::string>& results, bool skip_empty = true, bool trim = true);
    static void StringSplit(const std::wstring& str, const std::wstring& div_str, vector<std::wstring>& results, bool skip_empty = true, bool trim = true);

    static bool StringTransform(std::string& str, bool upper);
    static bool StringTransform(std::wstring& str, bool upper);

    //Read file contents
    static bool GetFileContent(const wchar_t* file_path, string& contents_buff, bool binary = true);

    //Read file contents
    //file_path: the path of the file
    //length: the length of the file
    //binary: whether to read in decimal mode
    //Return value: If the read is successful, a pointer of const char type of the read file content is returned. After use, this pointer needs to be released by itself using delete. Reading failure returns nullptr
    static const char* GetFileContent(const wchar_t* file_path, size_t& length, bool binary = true);

    /*Convert the data into a string in units of KB, MB, and GB according to the size of the data
    size: Number of bytes of data
    Return value: converted string
    */
    static CString DataSizeToString(unsigned long long size, const PublicSettingData& cfg);

    /*Convert the data into a string in units of KB, MB, and GB according to the size of the data
    size: Number of bytes of data
    with_space: Whether the value and unit are separated by spaces
    Return value: converted string
    */
    static CString DataSizeToString(unsigned long long size, bool with_space = true);

    //Convert temperature information to string
    static CString TemperatureToString(float temperature, const PublicSettingData& cfg);

    //Convert usage to string
    static CString UsageToString(int usage, const PublicSettingData& cfg);
    static CString FreqToString(float usage, const PublicSettingData& cfg);
    //static CString KBytesToString(unsigned int kb_size);
    static CString KBytesToString(unsigned __int64 kb_size);

    //Returns the time difference between two FILETIME structures
    static __int64 CompareFileTime2(FILETIME time1, FILETIME time2);

    //Write a log information str_text to the file_path file
    static void WriteLog(const char* str_text, LPCTSTR file_path);
    static void WriteLog(const wchar_t* str_text, LPCTSTR file_path);

    /*
    Function: Create a shortcut to the specified file in the specified directory
    Function parameters:
    lpszLnkFileDir  Specifies the directory and cannot be NULL.
    lpszFileName    Specify the file, NULL indicates the EXE file of the current process.
    lpszLnkFileName Shortcut name, NULL indicates EXE file name.
    wHotkey         0 means no shortcut keys are set
    pszDescription  Remarks
    iShowCmd        Run mode, default is regular window
    */
    static BOOL CreateFileShortcut(LPCTSTR lpszLnkFileDir, LPCTSTR lpszFileName = NULL, LPCTSTR lpszLnkFileName = NULL, LPCTSTR lpszWorkDir = NULL, WORD wHotkey = 0, LPCTSTR lpszDescription = NULL, int iShowCmd = SW_SHOWNORMAL);

    //Get the path to the "Startup" directory in the Start menu "All Programs"
    static wstring GetStartUpPath();

    //Get the file or folder under path and save the file or folder name in the files container.
    static void GetFiles(const wchar_t* path, vector<wstring>& files);

    //Get the file or folder under the path and call the function object func each time it is traversed
    //path: the path to search for
    //func: can be a function object or lambda expression, and the parameter is the file or folder name traversed
    static void GetFiles(const wchar_t* path, std::function<void(const wstring&)> func);

    //Determine whether a file exists
    static bool FileExist(LPCTSTR file_name);

    //Determine whether it is a folder
    static bool IsFolder(const wstring& path);

    static bool MoveAFile(LPCTSTR exist_file, LPCTSTR new_file);

    //Calculate the difference in time between two SYSTEMTIME structures (a-b, only hours, minutes and seconds are retained)
    static SYSTEMTIME CompareSystemTime(SYSTEMTIME a, SYSTEMTIME b);

    //Get the number of milliseconds since January 1, 1970 to the present
    static ULONGLONG GetCurrentTimeSinceEpochMilliseconds();

    //Get the directory of the current program
    static wstring GetModuleDir();

    //Get the path of the system32 folder
    static wstring GetSystemDir();

    //Get the path of the temporary folder
    static wstring GetTemplateDir();

    //Get the directory of Appdata/Local/TrafficMonitor. If it does not exist, it will be created automatically.
    static wstring GetAppDataConfigDir();

    //Draw text at specified location
    static void DrawWindowText(CDC* pDC, CRect rect, LPCTSTR lpszString, COLORREF color, COLORREF back_color);

    ////Set the clipping area of the drawing
    //static void SetDrawArea(CDC* pDC, CRect rect);

    /**
     * Determine whether the foreground window currently interacting with the user is full screen
     * @param[in] hMonitor The monitor to be judged (if empty, it is designated as the main monitor)
     * @return 
     */
    static bool IsForegroundFullscreen(HMONITOR hMonitor = NULL);

    //Save a string to the clipboard
    static bool CopyStringToClipboard(const wstring& str);

    static wstring GetJsonValueSimple(const wstring& json_str, const wstring& name);

    //Get the content of the URL
    static bool GetURL(const wstring& url, std::string& result, const wstring& user_agent = wstring());
    static bool GetURL(const wstring& url, wstring& result, bool utf8 = false, const wstring& user_agent = wstring());

    //Obtain external network IP address and IP location
    static void GetInternetIp(wstring& ip_address, wstring& ip_location, bool global);

    static void GetInternetIp2(wstring& ip_address, wstring& ip_location, bool ipv6);

    static void SetRect(CRect& rect, int x, int y, int width, int height);

    //Load strings from resource files. Among them, front_str and back_str are the strings that need to be added in front or behind when loading the string.
    static CString LoadText(const wchar_t* id, LPCTSTR back_str = nullptr);
    static CString LoadText(LPCTSTR front_str, const wchar_t* id, LPCTSTR back_str);

    //Safe format string, replace the string in the format_str in the shape of <% sequence number%> with the elements in the initialization list paras. The elements support the int/double/LPCTSTR/CString format, and the sequence number starts from 1
    static CString StringFormat(LPCTSTR format_str, const std::initializer_list<CVariant>& paras);

    //Load a string from the resource file and replace the string in the resource string in the form of <% sequence number%> with the parameters in the variable parameter list
    static CString LoadTextFormat(const wchar_t* id, const std::initializer_list<CVariant>& paras);

    //Convert int type to string
    //n: the value to be converted
    //thousand_separation: Whether to use commas to separate every 3 digits
    //is_unsigned: Whether the value is unsigned
    static CString IntToString(__int64 n, bool thousand_separation = false, bool is_unsigned = false);

    //Delete strings such as Bold and Light after the font name, and set the font thickness based on these strings
    static void NormalizeFont(LOGFONT& font);

    //Safe string copy functions
    static void WStringCopy(wchar_t* str_dest, int dest_size, const wchar_t* str_source, int source_size = INT_MAX);

    //String replacement
    static bool StringReplace(wstring& str, const wstring& str_old, const wstring& str_new);

    /// <summary>
    /// String similarity algorithm-edit distance method
    /// </summary>
    /// <returns>The returned value is 0~1, the larger the value, the higher the similarity</returns>
    static double StringSimilarDegree_LD(const std::string& srcString, const std::string& matchString);
    static double StringSimilarDegree_LD(const std::wstring& srcString, const std::wstring& matchString);
    
    //Set thread language
    static void SetThreadLanguage(WORD language);

    //Set color mode
    static void SetColorMode(ColorMode mode);

    //After testing, it seems that when the R and B values of the transparent color are equal, there will be a problem that the right-click menu cannot pop up when you right-click the taskbar window, but the system taskbar right-click menu pops up.
    //In order to solve this problem, the color values need to be converted
    //The function of this function is to determine whether the R and B values of a color are equal. If so, add 1 to the B value of the color (if B==255, subtract 1)
    static void TransparentColorConvert(COLORREF& transparent_color);

    static void SetDialogFont(CWnd* pDlg, CFont* pFont);

    //Load custom text resources from resources. id: resource ID, code_type: text encoding format: 0:ANSI, 1:UTF8, 2:UTF16
    static CString GetTextResource(UINT id, int code_type);

    //Read last compile time from resource file
    static CString GetLastCompileTime();

    //Load an icon from a resource
    static HICON LoadIconResource(UINT id, int size);

    //Get the serial number of a menu item
    static int GetMenuItemPosition(CMenu* pMenu, UINT id);

    //Load a menu from a resource file and handle text translation
    static void LoadMenuResource(CMenu& menu, UINT res_id);

    static bool IsColorSimilar(COLORREF color1, COLORREF color2);

    //Count the number of 1's in binary
    static int CountOneBits(unsigned int value);

    //Set a certain bit of a number
    static void SetNumberBit(unsigned int& num, int bit, bool value);

    //Get a certain bit of a number
    static bool GetNumberBit(unsigned int num, int bit);

    //Remove duplicate elements in vector without sorting
    template<class T>
    static void RemoveVectorDuplicateItem(vector<T>& vec)
    {
        std::set<T> si;
        for (auto it = vec.begin(); it != vec.end();)
        {
            if (si.count(*it) == 0)//Here it is judged whether the current element has appeared before. If it has not appeared before, the element will be retained and marked.
            {
                si.insert(*it);
                ++it;
            }
            else//If it has already appeared before, delete the current element
            {
                it = vec.erase(it);//Here delete the element pointed to by it
            }
        }
    }

    /**
     * @brief limits the range of a number
     * @param value The value to limit the range
     * @param min_value minimum value
     * @param max_value maximum value
     */
    template<class T>
    static void ValidatValue(T& value, const T& min_value, const T& max_value)
    {
        if (value < min_value)
            value = min_value;
        if (value > max_value)
            value = max_value;
    }

    //Get Windows theme colors
    static COLORREF GetWindowsThemeColor();

    static CString GetErrorMessage(DWORD error_code);
};

/**
 * @brief Get the array length at compile time
 *
 * @tparam T array element type
 * @tparam N array length inferred at compile time
 * @return constexpr std::size_t array length inferred at compile time
 */
template <typename T, std::size_t N>
constexpr std::size_t GetArrayLength(const T (&)[N]) noexcept
{
    return N;
}

/**
 * @brief The function executed by default before destructing the StaticVariableWrapper wrapped object, which actually has no operation.
 *
 * @tparam T
 */
template <class T>
class CDefaultStaticVariableWrapperDtor
{
public:
    void operator()(T*){};
};
/**
 * @brief is designed for static variable wrapper classes and is used to customize the default post-initialization behavior and pre-destruction behavior of variables.
 *
 * @tparam T The type to be wrapped
 * @tparam DTOR Customize the behavior before executing the destructor
 */
template <class T, class DTOR = CDefaultStaticVariableWrapperDtor<T>>
class CStaticVariableWrapper : private DTOR
{
private:
    T m_content;

public:
    /**
     * @brief constructs a StaticVariableWrapper
     *
     * @tparam CTOR Function type after default initialization of custom variables
     * @param ctor customizes the behavior of variables after default initialization, passing in the pointer of the variable as a parameter
     * @param dtor Customize the behavior of the variable before executing the destructor, and pass in the pointer of the variable as a parameter
     */
    template <class CTOR>
    CStaticVariableWrapper(CTOR ctor, DTOR dtor = {})
        : DTOR{dtor}
    {
        ctor(std::addressof(m_content));
    }
    ~CStaticVariableWrapper()
    {
        (*static_cast<DTOR*>(this))(std::addressof(m_content));
    }
    T& Get() noexcept
    {
        return m_content;
    }
    const T& Get() const noexcept
    {
        return m_content;
    }
};
/**
 * @brief function to generate static variable wrapper class
 *
 * @tparam T The type to be wrapped
 * @tparam CTOR Function type after default initialization of custom variables
 * @tparam DTOR Custom variable function type before executing the destructor
 * @param ctor customizes the behavior of variables after default initialization, passing in the pointer of the variable as a parameter
 * @param dtor Customize the behavior of the variable before executing the destructor, and pass in the pointer of the variable as a parameter
 * @return CStaticVariableWrapper<T, DTOR> The wrapped variable has been initialized
 */
template <class T, class CTOR, class DTOR = CDefaultStaticVariableWrapperDtor<T>>
auto MakeStaticVariableWrapper(CTOR ctor, DTOR dtor = {})
    -> CStaticVariableWrapper<T, DTOR>
{
    return {ctor, dtor};
}

/**
 * @brief calls the destructor of the corresponding type of the object pointed to by the pointer
 *
 * @tparam T The type passed in after removing the pointer
 * @param p_memory Pointer to the object whose destructor is to be executed
 */
template <class T>
void Destroy(T* p_memory)
{
    p_memory->~T();
}

template <class T, class... Args>
void EmplaceAt(T* p_memory, Args&&... args)
{
    ::new (p_memory) T(std::forward<Args>(args)...);
}
