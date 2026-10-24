#include "stdafx.h"
#include "DrawCommon.h"
#include "DrawCommonHelper.h"

CDrawCommon::CDrawCommon()
{
}

CDrawCommon::~CDrawCommon()
{
}

void CDrawCommon::Create(CDC* pDC, CWnd* pMainWnd)
{
    m_pDC = pDC;
    m_pMainWnd = pMainWnd;
    if (pMainWnd != nullptr)
        m_pfont = m_pMainWnd->GetFont();
    m_gdi_plus_drawer.Create(pDC);
}

void CDrawCommon::SetFont(CFont* pfont)
{
    m_pfont = pfont;
    m_pDC->SelectObject(m_pfont);
}

void CDrawCommon::SetDC(CDC* pDC)
{
    m_pDC = pDC;
    m_gdi_plus_drawer.Create(pDC);
}

void CDrawCommon::DrawWindowText(CRect rect, LPCTSTR lpszString, COLORREF color, Alignment align, bool draw_back_ground, bool multi_line, BYTE alpha)
{
    m_pDC->SetTextColor(color);
    if (!draw_back_ground)
        m_pDC->SetBkMode(TRANSPARENT);
    m_pDC->SelectObject(m_pfont);
    CSize text_size = m_pDC->GetTextExtent(lpszString);

    auto format = DrawCommonHelper::ProccessTextFormat(rect, text_size, align, multi_line);

    if (draw_back_ground)
        m_pDC->FillSolidRect(rect, m_back_color);
    m_pDC->DrawText(lpszString, rect, format);
}

void CDrawCommon::SetDrawRect(CRect rect)
{
    CRgn rgn;
    rgn.CreateRectRgnIndirect(rect);
    m_pDC->SelectClipRgn(&rgn);
}

void CDrawCommon::SetDrawRect(CDC* pDC, CRect rect)
{
    CRgn rgn;
    rgn.CreateRectRgnIndirect(rect);
    pDC->SelectClipRgn(&rgn);
}

void CDrawCommon::DrawBitmap(CBitmap& bitmap, CPoint start_point, CSize size, StretchMode stretch_mode)
{
    CDC memDC;

    //Get the actual size of the image
    BITMAP bm;
    GetObject(bitmap, sizeof(BITMAP), &bm);

    memDC.CreateCompatibleDC(m_pDC);
    memDC.SelectObject(&bitmap);
    // The following two lines avoid image distortion
    m_pDC->SetStretchBltMode(HALFTONE);
    m_pDC->SetBrushOrg(0, 0);

    DrawCommonHelper::ImageDrawAreaConvert(CSize(bm.bmWidth, bm.bmHeight), start_point, size, stretch_mode);

    m_pDC->StretchBlt(start_point.x, start_point.y, size.cx, size.cy, &memDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    memDC.DeleteDC();
}

void CDrawCommon::DrawBitmap(UINT bitmap_id, CPoint start_point, CSize size, StretchMode stretch_mode)
{
    CBitmap bitmap;
    bitmap.LoadBitmap(bitmap_id);
    DrawBitmap(bitmap, start_point, size, stretch_mode);
}

void CDrawCommon::DrawBitmap(HBITMAP hbitmap, CPoint start_point, CSize size, StretchMode stretch_mode, BYTE)
{
    CBitmap bitmap;
    if (!bitmap.Attach(hbitmap))
        return;
    DrawBitmap(bitmap, start_point, size, stretch_mode);
    bitmap.Detach();
}

void CDrawCommon::DrawIcon(HICON hIcon, CPoint start_point, CSize size)
{
    if (m_pDC->GetSafeHdc() == NULL)
        return;
    if (size.cx == 0 || size.cy == 0)
        ::DrawIconEx(m_pDC->GetSafeHdc(), start_point.x, start_point.y, hIcon, 0, 0, 0, NULL, DI_NORMAL | DI_DEFAULTSIZE);
    else
        ::DrawIconEx(m_pDC->GetSafeHdc(), start_point.x, start_point.y, hIcon, size.cx, size.cy, 0, NULL, DI_NORMAL);
}

void CDrawCommon::BitmapStretch(CImage* pImage, CImage* ResultImage, CSize size)
{
    if (pImage->IsDIBSection())
    {
        // Get the DC of pImage
        CDC* pImageDC1 = CDC::FromHandle(pImage->GetDC()); // Because Image has its own DC, it must use FromHandle to obtain the corresponding DC.

        CBitmap* bitmap1 = pImageDC1->GetCurrentBitmap();
        BITMAP bmpInfo;
        bitmap1->GetBitmap(&bmpInfo);

        // Create a new CImage
        ResultImage->Create(size.cx, size.cy, bmpInfo.bmBitsPixel);
        CDC* ResultImageDC = CDC::FromHandle(ResultImage->GetDC());

        // When the Destination is relatively small, it will be decided according to the Stretch Blt mode on the Destination DC whether to retain the information of the deleted point.
        ResultImageDC->SetStretchBltMode(HALFTONE);        // Use the highest quality methods
        ::SetBrushOrgEx(ResultImageDC->m_hDC, 0, 0, NULL); // Adjust the starting point of the Brush

        // Draw pImage onto ResultImage
        StretchBlt(*ResultImageDC, 0, 0, size.cx, size.cy, *pImageDC1, 0, 0, pImage->GetWidth(), pImage->GetHeight(), SRCCOPY);
        // pImage->Draw(*ResultImageDC,0,0,StretchWidth,StretchHeight,0,0,pImage->GetWidth(),pImage->GetHeight());

        pImage->ReleaseDC();
        ResultImage->ReleaseDC();
    }
}

void CDrawCommon::FillRect(CRect rect, COLORREF color, BYTE alpha)
{
    m_pDC->FillSolidRect(rect, color);
}

void CDrawCommon::FillRectWithBackColor(CRect rect)
{
    m_pDC->FillSolidRect(rect, m_back_color);
}

void CDrawCommon::DrawRectOutLine(CRect rect, COLORREF color, int width, bool dot_line, BYTE alpha, int radius)
{
    if (radius > 0)
    {
        //Draw a rounded rectangle when the radius is greater than 0, use GDI+ to draw it
        m_gdi_plus_drawer.DrawRectOutLine(rect, color, width, dot_line, alpha, radius);
    }
    else
    {
        CPen aPen, * pOldPen;
        aPen.CreatePen((dot_line ? PS_DOT : PS_SOLID), width, color);
        pOldPen = m_pDC->SelectObject(&aPen);
        CBrush* pOldBrush{ dynamic_cast<CBrush*>(m_pDC->SelectStockObject(NULL_BRUSH)) };

        rect.DeflateRect(width / 2, width / 2);
        m_pDC->Rectangle(rect);
        m_pDC->SelectObject(pOldPen);
        m_pDC->SelectObject(pOldBrush); // Restore the old brush
        aPen.DeleteObject();
    }
}

void CDrawCommon::GetRegionFromImage(CRgn& rgn, CBitmap& cBitmap, int threshold)
{
    CDC memDC;

    memDC.CreateCompatibleDC(NULL);
    CBitmap* pOldMemBmp = NULL;
    pOldMemBmp = memDC.SelectObject(&cBitmap);

    //Create the total form area, the initial region is 0
    rgn.CreateRectRgn(0, 0, 0, 0);

    BITMAP bit;
    cBitmap.GetBitmap(&bit); //Get the bitmap parameters, here you need to use the length and width of the bitmap
    int y;
    for (y = 0; y < bit.bmHeight; y++)
    {
        CRgn rgnTemp; //Save temporary region
        int iX = 0;
        do
        {
            //Skip the transparent color and find the next non-transparent color point.
            while (iX < bit.bmWidth && GetColorBritness(memDC.GetPixel(iX, y)) <= threshold)
                iX++;
            int iLeftX = iX; //Remember this starting point

            //Find the next transparent point
            while (iX < bit.bmWidth && GetColorBritness(memDC.GetPixel(iX, y)) > threshold)
                ++iX;

            //Create a temporary "region" containing a height of 1 pixel between the starting point and the focus
            rgnTemp.CreateRectRgn(iLeftX, y, iX, y + 1);
            rgn.CombineRgn(&rgn, &rgnTemp, RGN_OR);

            //Delete the temporary "region", otherwise an error will occur the next time it is created
            rgnTemp.DeleteObject();
        } while (iX < bit.bmWidth);
    }
    memDC.DeleteDC();
}

int CDrawCommon::GetColorBritness(COLORREF color)
{
    return (GetRValue(color) + GetGValue(color) + GetBValue(color)) / 3;
}

void CDrawCommon::DrawLine(CPoint start_point, CPoint end_point, COLORREF color, BYTE alpha)
{
    CPen aPen, *pOldPen;
    aPen.CreatePen(PS_SOLID, 1, color);
    pOldPen = m_pDC->SelectObject(&aPen);
    CBrush* pOldBrush{dynamic_cast<CBrush*>(m_pDC->SelectStockObject(NULL_BRUSH))};

    m_pDC->MoveTo(start_point);
    m_pDC->LineTo(end_point);
    m_pDC->SelectObject(pOldPen);
    m_pDC->SelectObject(pOldBrush); // Restore the old brush
    aPen.DeleteObject();
}

int CDrawCommon::GetTextWidth(LPCTSTR lpszString)
{
    return m_pDC->GetTextExtent(lpszString).cx;
}

void CDrawCommon::GetTextExtent(const wchar_t* lpszString, int& w, int& h)
{
    CSize size = m_pDC->GetTextExtent(lpszString);
    w = size.cx;
    h = size.cy;
}
