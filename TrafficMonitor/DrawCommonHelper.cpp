#include "stdafx.h"
#include "DrawCommonHelper.h"

UINT DrawCommonHelper::ProccessTextFormat(CRect rect, CSize text_length, IDrawCommon::Alignment align, bool multi_line) noexcept
{
    UINT result; // Text format of CDC::DrawText() function
    if (multi_line)
        result = DT_EDITCONTROL | DT_WORDBREAK | DT_NOPREFIX;
    else
        result = DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX;

    if (text_length.cx > rect.Width()) //If the text width exceeds the width of the rectangular area, left-justify when centering is set
    {
        if (align == IDrawCommon::Alignment::RIGHT)
            result |= DT_RIGHT;
    }
    else
    {
        switch (align)
        {
        case IDrawCommon::Alignment::RIGHT:
            result |= DT_RIGHT;
            break;
        case IDrawCommon::Alignment::CENTER:
            result |= DT_CENTER;
            break;
        }
    }
    return result;
}

void DrawCommonHelper::ImageDrawAreaConvert(CSize image_size, CPoint& start_point, CSize& size, IDrawCommon::StretchMode stretch_mode)
{
    if (size.cx == 0 || size.cy == 0)       //If the specified size is 0, the actual size of the bitmap is used to draw
    {
        size = CSize(image_size.cx, image_size.cy);
    }
    else
    {
        if (stretch_mode == IDrawCommon::StretchMode::FILL)
        {
            float w_h_ratio, w_h_ratio_draw;        //The aspect ratio of the image, the aspect ratio of the draw size
            w_h_ratio = static_cast<float>(image_size.cx) / image_size.cy;
            w_h_ratio_draw = static_cast<float>(size.cx) / size.cy;
            if (w_h_ratio > w_h_ratio_draw)     //If the aspect ratio of the image is greater than the aspect ratio of the drawing area, you need to crop the image on both sides
            {
                int image_width;        //scaled width
                image_width = image_size.cx * size.cy / image_size.cy;
                start_point.x -= ((image_width - size.cx) / 2);
                size.cx = image_width;
            }
            else
            {
                int image_height;       //scaled height
                image_height = image_size.cy * size.cx / image_size.cx;
                start_point.y -= ((image_height - size.cy) / 2);
                size.cy = image_height;
            }
        }
        else if (stretch_mode == IDrawCommon::StretchMode::FIT)
        {
            CSize draw_size = image_size;
            float w_h_ratio, w_h_ratio_draw;        //The aspect ratio of the image, the aspect ratio of the draw size
            w_h_ratio = static_cast<float>(image_size.cx) / image_size.cy;
            w_h_ratio_draw = static_cast<float>(size.cx) / size.cy;
            if (w_h_ratio > w_h_ratio_draw)     //If the aspect ratio of the image is greater than the aspect ratio of the drawing area
            {
                draw_size.cy = draw_size.cy * size.cx / draw_size.cx;
                draw_size.cx = size.cx;
                start_point.y += ((size.cy - draw_size.cy) / 2);
            }
            else
            {
                draw_size.cx = draw_size.cx * size.cy / draw_size.cy;
                draw_size.cy = size.cy;
                start_point.x += ((size.cx - draw_size.cx) / 2);
            }
            size = draw_size;
        }
    }
}

void DrawCommonHelper::GetBitmapAlphaPixel(HBITMAP hBitmap, std::set<Point>& points)
{
    points.clear();
    BITMAP bm;
    GetObject(hBitmap, sizeof(BITMAP), &bm);

    int width = bm.bmWidth;
    int height = bm.bmHeight;

    // Get the pixel data of the bitmap
    BITMAPINFO bmpInfo = { 0 };
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = width;
    bmpInfo.bmiHeader.biHeight = -height; // top-down DIB
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 32;
    bmpInfo.bmiHeader.biCompression = BI_RGB;

    HDC hdc = CreateCompatibleDC(NULL);
    SelectObject(hdc, hBitmap);

    // Allocate memory to store bitmap pixels
    RGBQUAD* pPixels = new RGBQUAD[width * height];
    GetDIBits(hdc, hBitmap, 0, height, pPixels, &bmpInfo, DIB_RGB_COLORS);

    // Traverse all pixels
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int index = y * width + x;
            //Add pixels with alpha value 0
            if (pPixels[index].rgbReserved == 0)
                points.insert(Point(x, y));
        }
    }

    delete[] pPixels;
    DeleteDC(hdc);

}

void DrawCommonHelper::FixBitmapTextAlpha(HBITMAP hBitmap, BYTE alpha, std::set<Point> alpha_points)
{
    BITMAP bm;
    GetObject(hBitmap, sizeof(BITMAP), &bm);

    int width = bm.bmWidth;
    int height = bm.bmHeight;

    // Get the pixel data of the bitmap
    BITMAPINFO bmpInfo = { 0 };
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = width;
    bmpInfo.bmiHeader.biHeight = -height; // top-down DIB
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 32;
    bmpInfo.bmiHeader.biCompression = BI_RGB;

    HDC hdc = CreateCompatibleDC(NULL);
    SelectObject(hdc, hBitmap);

    // Allocate memory to store bitmap pixels
    RGBQUAD* pPixels = new RGBQUAD[width * height];
    GetDIBits(hdc, hBitmap, 0, height, pPixels, &bmpInfo, DIB_RGB_COLORS);

    // Iterate through all pixels
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int index = y * width + x;
            //If an alpha value of 0 is detected but is not in alpha_points, correct it to the correct alpha value
            if (pPixels[index].rgbReserved == 0 && !alpha_points.contains(Point(x, y)))
                pPixels[index].rgbReserved = alpha; // Set alpha channel
        }
    }

    // Write modified pixel data back to bitmap
    SetDIBits(hdc, hBitmap, 0, height, pPixels, &bmpInfo, DIB_RGB_COLORS);

    delete[] pPixels;
    DeleteDC(hdc);
}
