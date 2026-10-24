#pragma once
#include "CommonData.h"
#include "TinyXml2Helper.h"
#include "DrawCommon.h"
#include <gdiplus.h>

class CSkinFile
{
public:
    CSkinFile();
    ~CSkinFile();


    //
    /**
     * @brief Load skin information from file
     * @param skin_name skin name
     */
    bool Load(const wstring& skin_name);

    //Skin information
    struct SkinInfo
    {
        std::vector<COLORREF> text_color;   //text color
        bool specify_each_item_color{};     //Whether to specify the color of each item
        wstring skin_author;                //author of skin
        FontInfo font_info;                 //Font information
        DispStrings display_text;    //Display text for each item

        COLORREF TextColor(size_t i) const
        {
            if (i >= 0 && i < text_color.size())
                return text_color[i];
            else if (!text_color.empty())
                return text_color.front();
            else
                return 0;
        }
    };

    //Alignment
    enum class Alignment
    {
        LEFT,       //left aligned
        RIGHT,      //Align right
        CENTER,     //center
        SIDE        //Justify
    };

    //Layout information for each item in the skin
    struct LayoutItem
    {
        int x{};            //X position
        int y{};            //Y position
        int width{};        //Width
        Alignment align{};  //Alignment
        bool show{};        //Whether to display
    };

    //Skin layout
    struct Layout
    {
        int width{};        //Width
        int height{};       //height
        std::map<CommonDisplayItem, LayoutItem> layout_items; //Layout information for each item
        LayoutItem GetItem(CommonDisplayItem display_item) const
        {
            auto iter = layout_items.find(display_item);
            if (iter != layout_items.end())
                return iter->second;
            return LayoutItem();
        }
    };

    //Skin layout information
    struct LayoutInfo
    {
        int text_height{};  //Skin text height
        bool no_label{};    //Whether to hide labels
        Layout layout_l;    //Layout when "Show more information"
        Layout layout_s;    //Layout without "Show more information"
    };

    //Skin preview information
    struct PreviewInfo
    {
        struct Pos
        {
            int x{};
            int y{};
        };
        int width{};    //Preview width
        int height{};   //Preview height
        Pos l_pos;      //The position of the window in the preview when "Show more information"
        Pos s_pos;      //The position of the window in the preview when "Show more information" is not enabled
    };

    const SkinInfo& GetSkinInfo() const { return m_skin_info; }
    const LayoutInfo& GetLayoutInfo() const { return m_layout_info; }
    const PreviewInfo& GetPreviewInfo() const { return m_preview_info; }

    const CImage& GetBackgroundL() const { return m_background_l; }
    const CImage& GetBackgroundS() const { return m_background_s; }

    bool IsPNG() const { return m_is_png; }
    void SetAlpha(int alpha);       //Set the opacity of the main window, alpha: 0~255, only valid when using png background

    void SetSettingData(const SkinSettingData& setting_data);

    //Draw preview
    //pDC: Plotted CDC
    //rect: drawing area
    void DrawPreview(CDC* pDC, CRect rect);

    //Draw the main interface
    void DrawInfo(CDC* pDC, bool show_more_info);

    static string GetDisplayItemXmlNodeName(DisplayItem display_item);

    //Get all displayed items on this skin
    void GetSkinDisplayItems(std::set<CommonDisplayItem>& skin_all_items) const;

private:
    bool LoadFromXml(const wstring& file_path);     //Read skin data from xml file
    void LoadFromIni(const wstring& file_path);     //Read skin data from ini file (for compatibility with older skins)

    CSkinFile::Layout LayoutFromXmlNode(tinyxml2::XMLElement* ele);

    struct DrawStr
    {
        CString label;
        CString value;
        CString GetStr() const
        {
            return label + value;
        }
    };

    static void DrawSkinText(IDrawCommon& drawer, DrawStr draw_str, CRect rect, COLORREF color, Alignment align);

    //Draw all display items in the main interface except the background image
    void DrawItemsInfo(IDrawCommon& drawer, Layout& layout, CFont& font) const;

private:
    SkinInfo m_skin_info;
    LayoutInfo m_layout_info;
    PreviewInfo m_preview_info;
    std::map<std::string, std::string> m_plugin_map;  //The mapping relationship between plug-in names and xml node names. key is the xml node name, value is the plug-in ID

    CFont m_font;
    CImage m_background_s;
    CImage m_background_l;
    bool m_is_png{};
    Gdiplus::Image* m_background_png_s{};
    Gdiplus::Image* m_background_png_l{};
    int m_alpha{ 255 };     //Opacity, only valid when the background is png
    SkinSettingData m_setting_data;
    bool m_is_error{ false };
};
