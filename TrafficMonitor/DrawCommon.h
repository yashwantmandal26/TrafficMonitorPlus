//Encapsulated drawing class
#pragma once
#include "IDrawCommon.h"
#include "D2D1Support.h"
#include "CommonData.h"
#include "Nullable.hpp"
#include "DrawCommonEx.h"

class CDrawCommon final : public IDrawCommon
{
public:

    CDrawCommon();
    ~CDrawCommon();

    void Create(CDC* pDC, CWnd* pMainWnd);
    void SetFont(CFont* pfont) override;		//Set the font for drawn text
    void SetDC(CDC* pDC);		//Set the DC for drawing
    virtual CDC* GetDC() override { return m_pDC; }
    void SetBackColor(COLORREF back_color, BYTE alpha = 255) override { m_back_color = back_color; }
    void SetTextColor(const COLORREF text_color, BYTE alpha = 255) override
    {
        m_pDC->SetTextColor(text_color);
    }

    void DrawWindowText(CRect rect, LPCTSTR lpszString, COLORREF color, Alignment align = Alignment::LEFT, bool draw_back_ground = false, bool multi_line = false, BYTE alpha = 255) override; //Draw text within a specified rectangular area

    void SetDrawRect(CRect rect) override;		//Set drawing clipping area
    static void SetDrawRect(CDC* pDC, CRect rect);

    //draw a bitmap
    void DrawBitmap(CBitmap& bitmap, CPoint start_point, CSize size, StretchMode stretch_mode = StretchMode::STRETCH);
    void DrawBitmap(UINT bitmap_id, CPoint start_point, CSize size, StretchMode stretch_mode = StretchMode::STRETCH);
    void DrawBitmap(HBITMAP hbitmap, CPoint start_point, CSize size, StretchMode stretch_mode = StretchMode::STRETCH, BYTE alpha = 255) override;

    void DrawIcon(HICON hIcon, CPoint start_point, CSize size) override;

    //Stretch the image to the specified size (https://blog.csdn.net/sichuanpb/article/details/22986877)
    static void BitmapStretch(CImage* pImage, CImage* ResultImage, CSize size);

    void FillRect(CRect rect, COLORREF color, BYTE alpha = 255) override; //Fill the rectangle with a solid color
    void FillRectWithBackColor(CRect rect);			//Fill rectangle with background color
    void DrawRectOutLine(CRect rect, COLORREF color, int width = 1, bool dot_line = false, BYTE alpha = 255, int radius = 0) override; //Draw a rectangular border. Dotted line if dot_line is true

    //Create an area from the image. If the brightness of the pixel is less than threshold (value is 0~255, 0 is black, 255 is white), then the pixel is outside the area.
    //https://blog.csdn.net/tajon1226/article/details/6589180
    static void GetRegionFromImage(CRgn& rgn, CBitmap& cBitmap, int threshold);

    void DrawLine(CPoint start_point, CPoint end_point, COLORREF color, BYTE alpha = 255) override; //Draw lines using current brush

    virtual int GetTextWidth(LPCTSTR lpszString) override;

    virtual void GetTextExtent(const wchar_t* lpszString, int& w, int& h) override;

private:
    CDC* m_pDC{};		//Pointer to CDC class used for drawing
    CWnd* m_pMainWnd{};	//The handle of the drawing window
    CFont* m_pfont{};
    COLORREF m_back_color{};
    CDrawCommonEx m_gdi_plus_drawer;

    static int GetColorBritness(COLORREF color);
};

