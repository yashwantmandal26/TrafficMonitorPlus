#pragma once
#include "CommonData.h"
#include "PluginInterface.h"

class IDrawBuffer
{
public:
    virtual ~IDrawBuffer() = default;
};

class IDrawCommon : public IPluginDrawer
{
public:

    virtual void SetBackColor(COLORREF back_color, BYTE alpha = 255) = 0;
    // Set the font for drawn text
    virtual void SetFont(CFont* pfont) = 0;
    // Draw text within a specified rectangular area
    virtual void DrawWindowText(CRect rect, LPCTSTR lpszString, COLORREF color, Alignment align = Alignment::LEFT, bool draw_back_ground = false, bool multi_line = false, BYTE alpha = 255) = 0;
    // Set drawing clipping area
    virtual void SetDrawRect(CRect rect) = 0;
    // Fill the rectangle with a solid color
    virtual void FillRect(CRect rect, COLORREF color, BYTE alpha = 255) = 0;
    // Draw a rectangular border. If dot_line is true, it is a dotted line. When radius is greater than 0, it is a rounded rectangle.
    virtual void DrawRectOutLine(CRect rect, COLORREF color, int width = 1, bool dot_line = false, BYTE alpha = 255, int radius = 0) = 0;
    // Draw lines using current brush
    virtual void DrawLine(CPoint start_point, CPoint end_point, COLORREF color, BYTE alpha = 255) = 0;
    // Use the current brush to draw a line, taking start_point as the starting point, and draw a line segment with a length of height upward.
    virtual void DrawLine(CPoint start_point, int height, COLORREF color, BYTE alpha = 255);
    virtual void SetTextColor(const COLORREF color, BYTE alpha = 255) = 0;
    // draw a bitmap
    virtual void DrawBitmap(HBITMAP hbitmap, CPoint start_point, CSize size, StretchMode stretch_mode = StretchMode::STRETCH, BYTE alpha = 255) = 0;
    virtual void DrawIcon(HICON hIcon, CPoint start_point, CSize size) = 0;
    virtual ~IDrawCommon() = default;

    //Gets the drawing context handle.
    virtual CDC* GetDC() = 0;
    //Get text width
    virtual int GetTextWidth(LPCTSTR lpszString) { return 0; }

    // Inherited through IPluginDrawer
    virtual int GetAPIVersion() override;
    virtual void DrawWindowText(int x, int y, int w, int h, const wchar_t* lpszString, unsigned long color, Alignment align, bool multi_line, unsigned char alpha) override;
    virtual void SetDrawRect(int x, int y, int w, int h) override;
    virtual void FillRect(int x, int y, int w, int h, unsigned long color, unsigned char alpha) override;
    virtual void DrawRectOutLine(int x, int y, int w, int h, unsigned long color, int width, bool dot_line, unsigned char alpha, int radius) override;
    virtual void DrawBitmap(void* hbitmap, int x, int y, int w, int h, StretchMode stretch_mode, unsigned char alpha) override;
    virtual void DrawIcon(void* hIcon, int x, int y, int w, int h) override;
    virtual void DrawLine(int x1, int y1, int x2, int y2, unsigned long color, int width = 1, bool dot_line = false, unsigned char alpha = 255) override;
    virtual void* GetHDC() override;
};

namespace DrawCommonHelper
{
    /**
     * @brief is an enumeration of renderer types. When adding a new renderer type, the enumeration should be added to it.
     *
     */
    enum class RenderType
    {
        // Using GDI
        DEFAULT,
        // If supported, use D2D1
        D2D1,
        // If the system is greater than win8.1, use DirectComposition to render the result when using D2D1
        D2D1_WITH_DCOMPOSITION
    };

    constexpr BYTE GDI_NO_MODIFIED_FLAG = 0x01;
    constexpr BYTE OPAQUE_ALPHA_VALUE = 0xFF;
    constexpr BYTE TRANSPARENT_ALPHA_VALUE = 0x00;
    constexpr BYTE AVAILABLE_MINIMUM_ALPHA = 0x02;
    constexpr BYTE GDI_MODIFIED_FLAG = 0x00;
}
