#include "stdafx.h"
#include "SkinFile.h"
#include "Common.h"
#include "FilePathHelper.h"
#include "TrafficMonitor.h"
#include "SettingsHelper.h"
#include "DrawCommon.h"
#include "DrawCommonEx.h"
#include "SkinManager.h"
#include "DrawCommonHelper.h"


CSkinFile::CSkinFile()
{
}


CSkinFile::~CSkinFile()
{
    SAFE_DELETE(m_background_png_s);
    SAFE_DELETE(m_background_png_l);
}

static CSkinFile::LayoutItem LayoutItemFromXmlNode(tinyxml2::XMLElement* ele)
{
    CSkinFile::LayoutItem layout_item;
    layout_item.x = theApp.DPI(atoi(CTinyXml2Helper::ElementAttribute(ele, "x")));
    layout_item.y = theApp.DPI(atoi(CTinyXml2Helper::ElementAttribute(ele, "y")));
    layout_item.width = theApp.DPI(atoi(CTinyXml2Helper::ElementAttribute(ele, "width")));
    layout_item.align = static_cast<CSkinFile::Alignment>(atoi(CTinyXml2Helper::ElementAttribute(ele, "align")));
    layout_item.show = CTinyXml2Helper::StringToBool(CTinyXml2Helper::ElementAttribute(ele, "show"));
    return layout_item;
}

CSkinFile::Layout CSkinFile::LayoutFromXmlNode(tinyxml2::XMLElement* ele)
{
    CSkinFile::Layout layout;
    layout.width = theApp.DPI(atoi(CTinyXml2Helper::ElementAttribute(ele, "width")));
    layout.height = theApp.DPI(atoi(CTinyXml2Helper::ElementAttribute(ele, "height")));
    CTinyXml2Helper::IterateChildNode(ele, [&](tinyxml2::XMLElement* ele_layout_item)
        {
            string str_layout_item = CTinyXml2Helper::ElementName(ele_layout_item);
            for (auto display_item : AllDisplayItems)
            {
                if (str_layout_item == CSkinFile::GetDisplayItemXmlNodeName(display_item))
                {
                    layout.layout_items[display_item] = LayoutItemFromXmlNode(ele_layout_item);
                    break;
                }
            }
            wstring plugin_id = CCommon::StrToUnicode(m_plugin_map[str_layout_item].c_str(), true);
            if (!plugin_id.empty())
            {
                for (const auto& plugin_item : theApp.m_plugins.GetPluginItems())
                {
                    if (plugin_id == plugin_item->GetItemId())
                    {
                        layout.layout_items[plugin_item] = LayoutItemFromXmlNode(ele_layout_item);
                        break;
                    }
                }
            }
        });
    return layout;
}

void CSkinFile::DrawSkinText(IDrawCommon& drawer, DrawStr draw_str, CRect rect, COLORREF color, Alignment align)
{
    int text_width = drawer.GetTextWidth(draw_str.GetStr());
    //Align labels and values
    if (align == Alignment::SIDE && text_width < rect.Width())      //Justify only if the text width is smaller than the width of the rectangle
    {
        //draw labels
        drawer.DrawWindowText(rect, draw_str.label, color, IDrawCommon::Alignment::LEFT);
        //Plot values
        drawer.DrawWindowText(rect, draw_str.value, color, IDrawCommon::Alignment::RIGHT);
    }
    else
    {
        IDrawCommon::Alignment text_align = IDrawCommon::Alignment::LEFT;
        if (align == Alignment::RIGHT)
            text_align = IDrawCommon::Alignment::RIGHT;
        else if (align == Alignment::CENTER)
            text_align = IDrawCommon::Alignment::CENTER;
        drawer.DrawWindowText(rect, draw_str.GetStr(), color, text_align);
    }
}

bool CSkinFile::Load(const wstring& skin_name)
{
    std::wstring file_path{ theApp.m_skin_path + skin_name + L"\\skin.xml" };
    if (!CCommon::FileExist(file_path.c_str()))
        file_path = theApp.m_skin_path + skin_name + L"\\skin.ini";
    CFilePathHelper file_path_helper{ file_path };
    wstring ext = file_path_helper.GetFileExtension();
    m_is_error = false;
    if (ext == L"ini")
    {
        LoadFromIni(file_path);
    }
    else
    {
        if (!LoadFromXml(file_path))
        {
            LoadFromIni(std::wstring());
            m_skin_info.text_color = { RGB(255, 255, 255) };
            m_is_error = true;
        }
    }

    CSkinManager::SkinSettingDataFronSkin(m_setting_data, *this);

    if (m_font.m_hObject)
        m_font.DeleteObject();

    //Create font object
    if (!m_skin_info.font_info.name.IsEmpty() && m_skin_info.font_info.size > 0)
    {
        m_skin_info.font_info.Create(m_font, theApp.GetDpi());
    }
    else
    {
        FontInfo font_info;
        font_info.name = theApp.m_str_table.GetLanguageInfo().default_font_name.c_str();
        font_info.size = 10;
        font_info.Create(m_font, theApp.GetDpi());
    }

    wstring path_dir = file_path_helper.GetDir();

    //Load png background image
    m_is_png = true;
    //First try to load png format, if failed, load bmp format.
    SAFE_DELETE(m_background_png_s);
    m_background_png_s = new Gdiplus::Image((path_dir + BACKGROUND_IMAGE_S_PNG).c_str());
    if (m_background_png_s->GetLastStatus() != Gdiplus::Ok)
    {
        m_is_png = false;
    }
    SAFE_DELETE(m_background_png_l);
    m_background_png_l = new Gdiplus::Image((path_dir + BACKGROUND_IMAGE_L_PNG).c_str());
    if (m_background_png_l->GetLastStatus() != Gdiplus::Ok)
    {
        m_is_png = false;
    }

    //Failed to load png background image, use bmp background image
    if (!m_is_png)
    {
        m_background_s.Destroy();
        m_background_s.Load((path_dir + BACKGROUND_IMAGE_S).c_str());
        m_background_l.Destroy();
        m_background_l.Load((path_dir + BACKGROUND_IMAGE_L).c_str());
    }
    return CCommon::FileExist(file_path.c_str());
}

bool CSkinFile::LoadFromXml(const wstring& file_path)
{
    m_skin_info = SkinInfo();
    m_layout_info = LayoutInfo();
    m_preview_info = PreviewInfo();
    std::map<std::string, std::wstring> display_text_map;  //Display the mapping relationship between the text of the project and the xml node. key is xml node, value is display text

    tinyxml2::XMLDocument doc;
    if (CTinyXml2Helper::LoadXmlFile(doc, file_path.c_str()))
    {
        CTinyXml2Helper::IterateChildNode(doc.FirstChildElement(), [&](tinyxml2::XMLElement* child) {
            string ele_name = CTinyXml2Helper::ElementName(child);
            //Read skin information
            if (ele_name == "skin")
            {
                CTinyXml2Helper::IterateChildNode(child, [&](tinyxml2::XMLElement* skin_item) {
                    string skin_item_name = CTinyXml2Helper::ElementName(skin_item);
                    //text color
                    if (skin_item_name == "text_color")
                    {
                        string str_text_color = CTinyXml2Helper::ElementText(skin_item);
                        std::vector<string> split_result;
                        CCommon::StringSplit(str_text_color, L',', split_result);
                        for (const auto& str : split_result)
                        {
                            m_skin_info.text_color.push_back(atoi(str.c_str()));
                        }
                        if (m_skin_info.text_color.size() < theApp.m_plugins.AllDisplayItemsWithPlugins().size())
                        {
                            COLORREF default_color{};
                            if (!m_skin_info.text_color.empty())
                                default_color = m_skin_info.text_color.front();
                            m_skin_info.text_color.resize(theApp.m_plugins.AllDisplayItemsWithPlugins().size(), default_color);
                        }
                    }
                    //Specify color for each item
                    else if (skin_item_name == "specify_each_item_color")
                    {
                        m_skin_info.specify_each_item_color = CTinyXml2Helper::StringToBool(CTinyXml2Helper::ElementText(skin_item));
                    }
                    //skin author
                    else if (skin_item_name == "skin_author")
                    {
                        m_skin_info.skin_author = CCommon::StrToUnicode(CTinyXml2Helper::ElementText(skin_item), true);
                    }
                    //font
                    else if (skin_item_name == "font")
                    {
                        m_skin_info.font_info.name = CCommon::StrToUnicode(CTinyXml2Helper::ElementAttribute(skin_item, "name"), true).c_str();
                        m_skin_info.font_info.size = atoi(CTinyXml2Helper::ElementAttribute(skin_item, "size"));
                        int font_style = atoi(CTinyXml2Helper::ElementAttribute(skin_item, "style"));
                        m_skin_info.font_info.bold = CCommon::GetNumberBit(font_style, 0);
                        m_skin_info.font_info.italic = CCommon::GetNumberBit(font_style, 1);
                        m_skin_info.font_info.underline = CCommon::GetNumberBit(font_style, 2);
                        m_skin_info.font_info.strike_out = CCommon::GetNumberBit(font_style, 3);
                    }
                    else if (skin_item_name == "display_text")
                    {
                        //Here first save all displayed text to display_text_map
                        CTinyXml2Helper::IterateChildNode(skin_item, [&](tinyxml2::XMLElement* display_text_item) {
                            string item_name = CTinyXml2Helper::ElementName(display_text_item);
                            wstring item_text = CCommon::StrToUnicode(CTinyXml2Helper::ElementText(display_text_item), true);
                            display_text_map[item_name] = item_text;
                        });
                    }
                });
            }
            //layout information
            else if (ele_name == "layout")
            {
                m_layout_info.text_height = theApp.DPI(atoi(CTinyXml2Helper::ElementAttribute(child, "text_height")));
                m_layout_info.no_label = CTinyXml2Helper::StringToBool(CTinyXml2Helper::ElementAttribute(child, "no_label"));
                CTinyXml2Helper::IterateChildNode(child, [this](tinyxml2::XMLElement* ele_layout) {
                    string str_layout = CTinyXml2Helper::ElementName(ele_layout);
                    if (str_layout == "layout_l")
                        m_layout_info.layout_l = LayoutFromXmlNode(ele_layout);
                    else if (str_layout == "layout_s")
                        m_layout_info.layout_s = LayoutFromXmlNode(ele_layout);
                });
            }
            //Preview
            else if (ele_name == "preview")
            {
                m_preview_info.width = theApp.DPI(atoi(CTinyXml2Helper::ElementAttribute(child, "width")));
                m_preview_info.height = theApp.DPI(atoi(CTinyXml2Helper::ElementAttribute(child, "height")));
                CTinyXml2Helper::IterateChildNode(child, [this](tinyxml2::XMLElement* ele_priview_item) {
                    string str_item_name = CTinyXml2Helper::ElementName(ele_priview_item);
                    if (str_item_name == "l")
                    {
                        m_preview_info.l_pos.x = theApp.DPI(atoi(CTinyXml2Helper::ElementAttribute(ele_priview_item, "x")));
                        m_preview_info.l_pos.y = theApp.DPI(atoi(CTinyXml2Helper::ElementAttribute(ele_priview_item, "y")));
                    }
                    else if (str_item_name == "s")
                    {
                        m_preview_info.s_pos.x = theApp.DPI(atoi(CTinyXml2Helper::ElementAttribute(ele_priview_item, "x")));
                        m_preview_info.s_pos.y = theApp.DPI(atoi(CTinyXml2Helper::ElementAttribute(ele_priview_item, "y")));
                    }
                });
            }
            //Plugin name mapping
            else if (ele_name == "plugin_map")
            {
                CTinyXml2Helper::IterateChildNode(child, [this](tinyxml2::XMLElement* plugin_item) {
                    string ele_name = CTinyXml2Helper::ElementName(plugin_item);
                    string ele_text = CTinyXml2Helper::ElementText(plugin_item);
                    m_plugin_map[ele_name] = ele_text;
                });
            }
        });
    }
    else
    {
        //xml parsing failed
        return false;
    }

    //Load display text
    for (const auto& display_text_item : display_text_map)
    {
        std::string node_name = display_text_item.first;
        std::wstring item_text = display_text_item.second;
        //Find built-in display items
        for (auto display_item : AllDisplayItems)
        {
            if (node_name == CSkinFile::GetDisplayItemXmlNodeName(display_item))
            {
                m_skin_info.display_text.Get(display_item) = item_text;
                break;
            }
        }
        //Find plugin projects
        for (const auto& item : m_plugin_map)
        {
            if (node_name == item.first && !item.second.empty())
            {
                std::wstring plugin_id = CCommon::StrToUnicode(item.second.c_str(), true);
                IPluginItem* plugin_item = theApp.m_plugins.GetItemById(plugin_id);
                if (plugin_item != nullptr)
                    m_skin_info.display_text.Get(plugin_item) = item_text;
            }
        }
    }
    return true;
}

void CSkinFile::LoadFromIni(const wstring& file_path)
{
    m_skin_info = SkinInfo();
    m_layout_info = LayoutInfo();
    m_preview_info = PreviewInfo();

    //Get skin information
    CSettingsHelper ini(file_path);
    //Get the text color of the current skin
    std::map<CommonDisplayItem, COLORREF> text_colors{};
    ini.LoadMainWndColors(_T("skin"), _T("text_color"), { TDI_UP, TDI_DOWN, TDI_CPU, TDI_MEMORY }, text_colors, 0);
    for (const auto& item : text_colors)
    {
        m_skin_info.text_color.push_back(item.second);
    }

    m_skin_info.specify_each_item_color = ini.GetBool(_T("skin"), _T("specify_each_item_color"), false);
    //Get the font of the current skin
    FontInfo default_font{};
    default_font.name = theApp.m_str_table.GetLanguageInfo().default_font_name.c_str();
    default_font.size = 10;
    ini.LoadFontData(L"skin", m_skin_info.font_info, default_font);
    //Get skin author
    m_skin_info.skin_author = ini.GetString(_T("skin"), _T("skin_author"), _T("unknow"));
    //Get display text
    auto getDisplayTextFromIni = [&](DisplayItem display_item, const wchar_t* key_name) {
        std::wstring str;
        if (ini.GetString(L"skin", key_name, str))
            m_skin_info.display_text.Get(display_item) = str;
    };
    getDisplayTextFromIni(TDI_UP, L"up_string");
    getDisplayTextFromIni(TDI_DOWN, L"down_string");
    getDisplayTextFromIni(TDI_CPU, L"cpu_string");
    getDisplayTextFromIni(TDI_MEMORY, L"memory_string");
    //Get preview area size
    m_preview_info.width = theApp.DPI(ini.GetInt(_T("layout"), _T("preview_width"), 238));
    m_preview_info.height = theApp.DPI(ini.GetInt(_T("layout"), _T("preview_height"), 105));

    //Read skin layout from ini file and scale it according to DPI
    m_layout_info.text_height = theApp.DPI(ini.GetInt(_T("layout"), _T("text_height"), 20));
    m_layout_info.no_label = ini.GetBool(_T("layout"), _T("no_text"), false);

    m_layout_info.layout_l.width = theApp.DPI(ini.GetInt(_T("layout"), _T("width_l"), 220));
    m_layout_info.layout_l.height = theApp.DPI(ini.GetInt(_T("layout"), _T("height_l"), 43));
    m_layout_info.layout_l.layout_items[TDI_UP].x = theApp.DPI(ini.GetInt(_T("layout"), _T("up_x_l"), 6));
    m_layout_info.layout_l.layout_items[TDI_UP].y = theApp.DPI(ini.GetInt(_T("layout"), _T("up_y_l"), 2));
    m_layout_info.layout_l.layout_items[TDI_UP].width = theApp.DPI(ini.GetInt(_T("layout"), _T("up_width_l"), 108));
    m_layout_info.layout_l.layout_items[TDI_UP].align = static_cast<Alignment>(ini.GetInt(_T("layout"), _T("up_align_l"), 0));
    m_layout_info.layout_l.layout_items[TDI_DOWN].x = theApp.DPI(ini.GetInt(_T("layout"), _T("down_x_l"), 114));
    m_layout_info.layout_l.layout_items[TDI_DOWN].y = theApp.DPI(ini.GetInt(_T("layout"), _T("down_y_l"), 2));
    m_layout_info.layout_l.layout_items[TDI_DOWN].width = theApp.DPI(ini.GetInt(_T("layout"), _T("down_width_l"), 110));
    m_layout_info.layout_l.layout_items[TDI_DOWN].align = static_cast<Alignment>(ini.GetInt(_T("layout"), _T("down_align_l"), 0));
    m_layout_info.layout_l.layout_items[TDI_CPU].x = theApp.DPI(ini.GetInt(_T("layout"), _T("cpu_x_l"), 6));
    m_layout_info.layout_l.layout_items[TDI_CPU].y = theApp.DPI(ini.GetInt(_T("layout"), _T("cpu_y_l"), 21));
    m_layout_info.layout_l.layout_items[TDI_CPU].width = theApp.DPI(ini.GetInt(_T("layout"), _T("cpu_width_l"), 108));
    m_layout_info.layout_l.layout_items[TDI_CPU].align = static_cast<Alignment>(ini.GetInt(_T("layout"), _T("cpu_align_l"), 0));
    m_layout_info.layout_l.layout_items[TDI_MEMORY].x = theApp.DPI(ini.GetInt(_T("layout"), _T("memory_x_l"), 114));
    m_layout_info.layout_l.layout_items[TDI_MEMORY].y = theApp.DPI(ini.GetInt(_T("layout"), _T("memory_y_l"), 21));
    m_layout_info.layout_l.layout_items[TDI_MEMORY].width = theApp.DPI(ini.GetInt(_T("layout"), _T("memory_width_l"), 110));
    m_layout_info.layout_l.layout_items[TDI_MEMORY].align = static_cast<Alignment>(ini.GetInt(_T("layout"), _T("memory_align_l"), 0));
    m_layout_info.layout_l.layout_items[TDI_UP].show = ini.GetBool(_T("layout"), _T("show_up_l"), true);
    m_layout_info.layout_l.layout_items[TDI_DOWN].show = ini.GetBool(_T("layout"), _T("show_down_l"), true);
    m_layout_info.layout_l.layout_items[TDI_CPU].show = ini.GetBool(_T("layout"), _T("show_cpu_l"), true);
    m_layout_info.layout_l.layout_items[TDI_MEMORY].show = ini.GetBool(_T("layout"), _T("show_memory_l"), true);
    m_preview_info.l_pos.x = theApp.DPI(ini.GetInt(_T("layout"), _T("preview_x_l"), 0));
    m_preview_info.l_pos.y = theApp.DPI(ini.GetInt(_T("layout"), _T("preview_y_l"), 47));

    m_layout_info.layout_s.width = theApp.DPI(ini.GetInt(_T("layout"), _T("width_s"), 220));
    m_layout_info.layout_s.height = theApp.DPI(ini.GetInt(_T("layout"), _T("height_s"), 28));
    m_layout_info.layout_s.layout_items[TDI_UP].x = theApp.DPI(ini.GetInt(_T("layout"), _T("up_x_s"), 6));
    m_layout_info.layout_s.layout_items[TDI_UP].y = theApp.DPI(ini.GetInt(_T("layout"), _T("up_y_s"), 4));
    m_layout_info.layout_s.layout_items[TDI_UP].width = theApp.DPI(ini.GetInt(_T("layout"), _T("up_width_s"), 108));
    m_layout_info.layout_s.layout_items[TDI_UP].align = static_cast<Alignment>(ini.GetInt(_T("layout"), _T("up_align_s"), 0));
    m_layout_info.layout_s.layout_items[TDI_DOWN].x = theApp.DPI(ini.GetInt(_T("layout"), _T("down_x_s"), 114));
    m_layout_info.layout_s.layout_items[TDI_DOWN].y = theApp.DPI(ini.GetInt(_T("layout"), _T("down_y_s"), 4));
    m_layout_info.layout_s.layout_items[TDI_DOWN].width = theApp.DPI(ini.GetInt(_T("layout"), _T("down_width_s"), 110));
    m_layout_info.layout_s.layout_items[TDI_DOWN].align = static_cast<Alignment>(ini.GetInt(_T("layout"), _T("down_align_s"), 0));
    m_layout_info.layout_s.layout_items[TDI_CPU].x = theApp.DPI(ini.GetInt(_T("layout"), _T("cpu_x_s"), 0));
    m_layout_info.layout_s.layout_items[TDI_CPU].y = theApp.DPI(ini.GetInt(_T("layout"), _T("cpu_y_s"), 0));
    m_layout_info.layout_s.layout_items[TDI_CPU].width = theApp.DPI(ini.GetInt(_T("layout"), _T("cpu_width_s"), 0));
    m_layout_info.layout_s.layout_items[TDI_CPU].align = static_cast<Alignment>(ini.GetInt(_T("layout"), _T("cpu_align_s"), 0));
    m_layout_info.layout_s.layout_items[TDI_MEMORY].x = theApp.DPI(ini.GetInt(_T("layout"), _T("memory_x_s"), 0));
    m_layout_info.layout_s.layout_items[TDI_MEMORY].y = theApp.DPI(ini.GetInt(_T("layout"), _T("memory_y_s"), 0));
    m_layout_info.layout_s.layout_items[TDI_MEMORY].width = theApp.DPI(ini.GetInt(_T("layout"), _T("memory_width_s"), 0));
    m_layout_info.layout_s.layout_items[TDI_MEMORY].align = static_cast<Alignment>(ini.GetInt(_T("layout"), _T("memory_align_s"), 0));
    m_layout_info.layout_s.layout_items[TDI_UP].show = ini.GetBool(_T("layout"), _T("show_up_s"), true);
    m_layout_info.layout_s.layout_items[TDI_DOWN].show = ini.GetBool(_T("layout"), _T("show_down_s"), true);
    m_layout_info.layout_s.layout_items[TDI_CPU].show = ini.GetBool(_T("layout"), _T("show_cpu_s"), false);
    m_layout_info.layout_s.layout_items[TDI_MEMORY].show = ini.GetBool(_T("layout"), _T("show_memory_s"), false);
    m_preview_info.s_pos.x = theApp.DPI(ini.GetInt(_T("layout"), _T("preview_x_s"), 0));
    m_preview_info.s_pos.y = theApp.DPI(ini.GetInt(_T("layout"), _T("preview_y_s"), 0));
}


void CSkinFile::SetAlpha(int alpha)
{
    m_alpha = alpha;
}

void CSkinFile::SetSettingData(const SkinSettingData& setting_data)
{
    //If the font changes, recreate the font
    if (m_setting_data.font != setting_data.font)
    {
        if (m_font.m_hObject)   //If m_font is already associated with a font resource object, release it
            m_font.DeleteObject();
        setting_data.font.Create(m_font, theApp.GetDpi());
    }
    m_setting_data = setting_data;
}

void CSkinFile::DrawPreview(CDC* pDC, CRect rect)
{
    CDrawCommon draw;
    draw.Create(pDC, nullptr);
    //Set font
    draw.SetFont(&m_font);
    if (m_is_error)
    {
        //Show error message
        draw.DrawWindowText(rect, CCommon::LoadText(IDS_SKIN_FILE_ERROR_INFO), RGB(252, 128, 45));
    }
    else
    {
        //draw background
        CRect rect_s(CPoint(m_preview_info.s_pos.x, m_preview_info.s_pos.y), CSize(m_layout_info.layout_s.width, m_layout_info.layout_s.height));
        CRect rect_l(CPoint(m_preview_info.l_pos.x, m_preview_info.l_pos.y), CSize(m_layout_info.layout_l.width, m_layout_info.layout_l.height));
        if (IsPNG())    //The png background is drawn using GDI+
        {
            CDrawCommonEx gdiplus_drawer(pDC);
            gdiplus_drawer.DrawImage(m_background_png_s, rect_s.TopLeft(), rect_s.Size(), IDrawCommon::StretchMode::STRETCH);
            gdiplus_drawer.DrawImage(m_background_png_l, rect_l.TopLeft(), rect_l.Size(), IDrawCommon::StretchMode::STRETCH);
        }
        else
        {
            if (m_background_s.IsNull())
                draw.FillRect(rect_s, RGB(230, 230, 230));
            else
                draw.DrawBitmap(m_background_s, rect_s.TopLeft(), rect_s.Size());
            if (m_background_l.IsNull())
                draw.FillRect(rect_l, RGB(230, 230, 230));
            draw.DrawBitmap(m_background_l, rect_l.TopLeft(), rect_l.Size());
        }

        std::set<CommonDisplayItem> all_skin_items;
        GetSkinDisplayItems(all_skin_items);

        //Get the text displayed for each item
        std::map<DisplayItem, DrawStr> map_str;
        for (auto iter = all_skin_items.begin(); iter != all_skin_items.end(); ++iter)
        {
            if (iter->IsPlugin())
                continue;
            DrawStr draw_str;
            draw_str.value = CommonDisplayItem(iter->ItemType()).GetItemValueSampleText(true);
            if (!m_layout_info.no_label)
            {
                if (m_setting_data.disp_str.IsInvalid())
                    draw_str.label = iter->DefaultString(true).c_str();
                else
                    draw_str.label = m_setting_data.disp_str.GetConst(*iter).c_str();
            }
            map_str[iter->ItemType()] = draw_str;
        }

        //Get text color
        std::map<CommonDisplayItem, COLORREF> text_colors{};
        if (m_setting_data.specify_each_item_color)
        {
            text_colors = m_setting_data.text_colors;
        }
        else if (!m_setting_data.text_colors.empty())
        {
            for (const auto& item : all_skin_items)
            {
                text_colors[item] = m_setting_data.text_colors.begin()->second;
            }
        }

        //Draw preview text
        auto drawPreviewText = [&](const Layout& layout, const PreviewInfo::Pos& pos) {
            for (auto iter = map_str.begin(); iter != map_str.end(); ++iter)
            {
                auto layout_iter = layout.layout_items.find(iter->first);
                if (layout_iter == layout.layout_items.end())
                    continue;
                auto layout_item = layout_iter->second;
                if (layout_item.show)
                {
                    CPoint point;
                    point.SetPoint(layout_item.x, layout_item.y);
                    point.Offset(pos.x, pos.y);
                    CRect rect(point, CSize(layout_item.width, m_layout_info.text_height));
                    COLORREF text_color{};
                    text_color = text_colors[iter->first];
                    DrawSkinText(draw, iter->second, rect, text_color, layout_item.align);
                }
            }

            //Draw plugin project
            for (const auto& plugin_item : theApp.m_plugins.GetPluginItems())
            {
                LayoutItem layout_item = layout.GetItem(plugin_item);
                if (layout_item.show)
                {
                    COLORREF cl{};
                    auto iter = text_colors.find(plugin_item);
                    if (iter != text_colors.end())
                        cl = iter->second;
                    else if (!text_colors.empty())
                        cl = text_colors.begin()->second;
                    //Rectangular area
                    CPoint point;
                    point.SetPoint(layout_item.x, layout_item.y);
                    point.Offset(pos.x, pos.y);
                    CRect rect(point, CSize(layout_item.width, m_layout_info.text_height));
                    ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(plugin_item);
                    if (plugin != nullptr && plugin->GetAPIVersion() >= 2)
                    {
                        plugin->OnExtenedInfo(ITMPlugin::EI_DRAW_TASKBAR_WND, L"0");
                    }
                    if (plugin_item->IsCustomDraw())
                    {
                        int brightness{ (GetRValue(cl) + GetGValue(cl) + GetBValue(cl)) / 2 };
                        if (plugin != nullptr && plugin->GetAPIVersion() >= 2)
                        {
                            plugin->OnExtenedInfo(ITMPlugin::EI_VALUE_TEXT_COLOR, std::to_wstring(cl).c_str());
                        }
                        draw.GetDC()->SetTextColor(cl);
                        plugin_item->DrawItem(draw.GetDC()->GetSafeHdc(), point.x, point.y, layout_item.width, m_layout_info.text_height, brightness >= 128);
                        if (plugin != nullptr && plugin->GetAPIVersion() >= 8)
                            plugin_item->DrawItemEx(&draw, rect.left, rect.top, rect.Width(), rect.Height(), brightness >= 128);
                    }
                    else
                    {
                        //draw text
                        DrawStr draw_str;
                        if (m_skin_info.display_text.IsInvalid())
                            draw_str.label = plugin_item->GetItemLableText();
                        else
                            draw_str.label = m_skin_info.display_text.GetConst(plugin_item).c_str();
                        draw_str.value = plugin_item->GetItemValueSampleText();
                        DrawSkinText(draw, draw_str, rect, cl, layout_item.align);
                    }
                }
            }
        };

        //Draw small preview text
        drawPreviewText(m_layout_info.layout_s, m_preview_info.s_pos);
        //Draw large preview text
        drawPreviewText(m_layout_info.layout_l, m_preview_info.l_pos);
    }
}

void CSkinFile::DrawInfo(CDC* pDC, bool show_more_info)
{
    //Draw background image
    Layout& layout{ show_more_info ? m_layout_info.layout_l : m_layout_info.layout_s };

    CRect rect(CPoint(0, 0), CSize(layout.width, layout.height));

    //If the background is in PNG format, use the UpdateLayeredWindow function to support transparent backgrounds, and display items using GDI+ to draw
    if (IsPNG())
    {
        HDC hdcMemory = CreateCompatibleDC(pDC->GetSafeHdc());
        HBITMAP hBitMap = CreateCompatibleBitmap(pDC->GetSafeHdc(), rect.Width(), rect.Height());
        SelectObject(hdcMemory, hBitMap);

        BLENDFUNCTION bf{};
        bf.BlendOp = AC_SRC_OVER;
        bf.BlendFlags = 0;
        bf.AlphaFormat = 1;
        bf.SourceConstantAlpha = m_alpha; // The maximum window transparency is 255 and the minimum is 0

        HWND hWnd = theApp.m_pMainWnd->GetSafeHwnd();
        //draw background
        CDrawCommonEx gdiplus_drawer;
        gdiplus_drawer.Create(CDC::FromHandle(hdcMemory));
        Gdiplus::Image* background_image{ show_more_info ? m_background_png_l : m_background_png_s };
        gdiplus_drawer.DrawImage(background_image, CPoint(0, 0), rect.Size(), CDrawCommon::StretchMode::FILL);
        
        //Save fully transparent pixels
        std::set<DrawCommonHelper::Point> alpha_points;
        DrawCommonHelper::GetBitmapAlphaPixel(hBitMap, alpha_points);

        //Draw display items
        DrawItemsInfo(gdiplus_drawer, layout, m_font);

        //Find the point where the display item is opaque before drawing, but becomes transparent after drawing, and correct its alpha value
        DrawCommonHelper::FixBitmapTextAlpha(hBitMap, m_alpha, alpha_points);

        SIZE sizeWindow = rect.Size();
        POINT ptSrc = { 0,0 };
        ::UpdateLayeredWindow(hWnd, pDC->GetSafeHdc(), nullptr, &sizeWindow, hdcMemory, &ptSrc, 0, &bf, ULW_ALPHA);

        gdiplus_drawer.GetGraphics()->ReleaseHDC(hdcMemory);
        DeleteObject(hBitMap);
        DeleteDC(hdcMemory);
    }
    //bmp format background, use gdi to draw the background and display items
    else
    {
        CDrawDoubleBuffer draw_double_buffer(pDC, rect);
        CDrawCommon draw;
        draw.Create(draw_double_buffer.GetMemDC(), nullptr);

        CImage& background_image{ show_more_info ? m_background_l : m_background_s };
        draw.DrawBitmap(background_image, CPoint(0, 0), CSize(layout.width, layout.height));

        DrawItemsInfo(draw, layout, m_font);
    }
}


void CSkinFile::DrawItemsInfo(IDrawCommon& drawer, Layout& layout, CFont& font) const
{
    //Get the text and numeric text displayed for each item
    std::map<DisplayItem, DrawStr> map_str;
    for (const auto& display_item : AllDisplayItems)
    {
        if (!m_layout_info.no_label)
            map_str[display_item].label = theApp.m_main_wnd_data.disp_str.GetConst(display_item).c_str();
        map_str[display_item].value = CommonDisplayItem(display_item).GetItemValueText(true);
    }

    //Get text color
    std::map<CommonDisplayItem, COLORREF> text_colors{};
    if (theApp.m_main_wnd_data.specify_each_item_color)
    {
        text_colors = theApp.m_main_wnd_data.text_colors;
    }
    else if (!theApp.m_main_wnd_data.text_colors.empty())
    {
        for (const auto& item : theApp.m_plugins.AllDisplayItemsWithPlugins())
        {
            text_colors[item] = theApp.m_main_wnd_data.text_colors.begin()->second;
        }
    }

    //draw text
    drawer.SetFont(&font);

    //draw text
    int index{};
    for (auto iter = map_str.begin(); iter != map_str.end(); ++iter)
    {
        const auto& layout_item = layout.GetItem(iter->first);
        if (layout_item.show)
        {
            //Rectangular area
            CRect rect(CPoint(layout_item.x, layout_item.y), CSize(layout_item.width, m_layout_info.text_height));

            //text color
            COLORREF text_color = text_colors[iter->first];

            //draw text
            DrawSkinText(drawer, map_str[iter->first], rect, text_color, layout_item.align);
        }
        index++;
    }

    //Draw plugin project
    for (const auto& plugin_item : theApp.m_plugins.GetPluginItems())
    {
        const auto& layout_item = layout.GetItem(plugin_item);
        if (layout_item.show)
        {
            //Plug-in project self-drawing
            COLORREF cl{};
            auto iter = text_colors.find(plugin_item);
            if (iter != text_colors.end())
                cl = iter->second;
            else if (!text_colors.empty())
                cl = text_colors.begin()->second;
            ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(plugin_item);
            if (plugin != nullptr && plugin->GetAPIVersion() >= 2)
            {
                plugin->OnExtenedInfo(ITMPlugin::EI_DRAW_TASKBAR_WND, L"0");
            }
            if (plugin_item->IsCustomDraw())
            {
                int brightness{ (GetRValue(cl) + GetGValue(cl) + GetBValue(cl)) / 2 };
                ITMPlugin* plugin = theApp.m_plugins.GetPluginByItem(plugin_item);
                if (plugin != nullptr && plugin->GetAPIVersion() >= 2)
                {
                    plugin->OnExtenedInfo(ITMPlugin::EI_VALUE_TEXT_COLOR, std::to_wstring(cl).c_str());
                }
                drawer.GetDC()->SetTextColor(cl);
                drawer.GetDC()->SetBkMode(TRANSPARENT);
                plugin_item->DrawItem(drawer.GetDC()->GetSafeHdc(), layout_item.x, layout_item.y, layout_item.width, m_layout_info.text_height, brightness >= 128);
                if (plugin != nullptr && plugin->GetAPIVersion() >= 8)
                    plugin_item->DrawItemEx(&drawer, layout_item.x, layout_item.y, layout_item.width, m_layout_info.text_height, brightness >= 128);
            }
            else
            {
                //Rectangular area
                CRect rect(CPoint(layout_item.x, layout_item.y), CSize(layout_item.width, m_layout_info.text_height));

                //draw text
                DrawStr draw_str;
                draw_str.label = theApp.m_main_wnd_data.disp_str.GetConst(plugin_item).c_str();
                draw_str.value = plugin_item->GetItemValueText();
                DrawSkinText(drawer, draw_str, rect, cl, layout_item.align);
            }
        }
    }
}


string CSkinFile::GetDisplayItemXmlNodeName(DisplayItem display_item)
{
    switch (display_item)
    {
    case TDI_UP:
        return "up";
        break;
    case TDI_DOWN:
        return "down";
        break;
    case TDI_TOTAL_SPEED:
        return "total_speed";
        break;
    case TDI_TODAY_TRAFFIC:
        return "today_traffic";
        break;
    case TDI_CPU:
        return "cpu";
        break;
    case TDI_MEMORY:
        return "memory";
        break;
    case TDI_GPU_USAGE:
        return "gpu";
        break;
    case TDI_CPU_TEMP:
        return "cpu_temperature";
        break;
    case TDI_GPU_TEMP:
        return "gpu_temperature";
        break;
    case TDI_HDD_TEMP:
        return "hdd_temperature";
        break;
    case TDI_MAIN_BOARD_TEMP:
        return "main_board_temperature";
        break;
    case TDI_HDD_USAGE:
        return "hdd";
        break;
    case TDI_CPU_FREQ:
        return "cpu_freq";
        break;
    default:
        return string();
        break;
    }
}

void CSkinFile::GetSkinDisplayItems(std::set<CommonDisplayItem>& skin_all_items) const
{
    skin_all_items.clear();
    for (const auto& layout_items : GetLayoutInfo().layout_l.layout_items)
        skin_all_items.insert(layout_items.first);
    for (const auto& layout_items : GetLayoutInfo().layout_s.layout_items)
        skin_all_items.insert(layout_items.first);
}
