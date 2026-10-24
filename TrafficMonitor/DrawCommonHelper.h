#pragma once
#include "IDrawCommon.h"

//Classes for double-buffered drawing
class CDrawDoubleBuffer final : public IDrawBuffer
{
public:
    CDrawDoubleBuffer(CDC* pDC, CRect rect)
        : m_pDC(pDC), m_rect(rect)
    {
        if (m_pDC != nullptr)
        {
            m_memDC.CreateCompatibleDC(NULL);
            m_memBitmap.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());
            m_pOldBit = m_memDC.SelectObject(&m_memBitmap);
        }
    }

    ~CDrawDoubleBuffer()
    {
        if (m_pDC != nullptr)
        {
            m_pDC->BitBlt(m_rect.left, m_rect.top, m_rect.Width(), m_rect.Height(), &m_memDC, 0, 0, SRCCOPY);
            m_memDC.SelectObject(m_pOldBit);
            m_memBitmap.DeleteObject();
            m_memDC.DeleteDC();
        }
    }

    CDC* GetMemDC()
    {
        return &m_memDC;
    }

private:
    CDC* m_pDC{};
    CDC m_memDC;
    CBitmap m_memBitmap;
    CBitmap* m_pOldBit{};
    CRect m_rect;
};

namespace DrawCommonHelper
{
    UINT ProccessTextFormat(CRect rect, CSize text_length, IDrawCommon::Alignment align, bool multi_line) noexcept;

    //Calculate the actual position of drawing the picture according to the picture stretching mode
    //image_size[int]: original size of the image
    //start_point[int][out]: starting position of the drawing area
    //size[int][out]: size of drawing area
    //stretch_mode[int]: stretch mode
    void ImageDrawAreaConvert(CSize image_size, CPoint& start_point, CSize& size, IDrawCommon::StretchMode stretch_mode);

    struct Point
    {
    public:
        Point() {}
        Point(int x, int y)
            : m_x(x), m_y(y)
        {}
        bool operator==(const Point& a) const
        {
            return m_x == a.m_x && m_y == a.m_y;
        }
        bool operator<(const Point& a) const
        {
            if (m_x == a.m_x)
                return m_y < a.m_y;
            else
                return m_x < a.m_x;
        }

    private:
        int m_x{};
        int m_y{};
    };

    //Get a completely transparent point in a position and save it to points
    void GetBitmapAlphaPixel(HBITMAP hBitmap, std::set<Point>& points);

    //Fix alpha channel of text portion in bitmap
    //After using UpdateLayeredWindow, the text drawn using GDI will also become transparent. This function will traverse the pixels in the bitmap with an alpha value of 0 but not in alpha_points, and correct them to the correct alpha value.
    void FixBitmapTextAlpha(HBITMAP hBitmap, BYTE alpha, std::set<Point> alpha_points);
};

