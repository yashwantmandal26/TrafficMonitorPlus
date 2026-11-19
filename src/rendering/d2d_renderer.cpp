// TrafficMonitorPlus - Direct2D Renderer Implementation
#include "pch.h"
#include "rendering/d2d_renderer.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace tmp {

D2DRenderer::~D2DRenderer() {
    DiscardDeviceResources();
    ReleaseLayeredBuffer();
}

bool D2DRenderer::Init(HWND hwnd, Surface surface) {
    DiscardDeviceResources();
    ReleaseLayeredBuffer();
    m_hwnd = hwnd;
    m_surface = surface;
    // Configure redirected presentation once per HWND, never during a repaint
    // or graphics-resource recovery (which must preserve the last image).
    if(m_surface==Surface::BufferedLayered&&
       !SetLayeredWindowAttributes(m_hwnd,RGB(12,12,12),0,LWA_COLORKEY))return false;

    // Create D2D Factory (single-threaded for simplicity)
    HRESULT hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        __uuidof(ID2D1Factory1),
        reinterpret_cast<void**>(m_factory.ReleaseAndGetAddressOf()));
    if (FAILED(hr)) return false;

    // Create DirectWrite factory
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(m_writeFactory.ReleaseAndGetAddressOf()));
    if (FAILED(hr)) return false;

    // Create default text format
    hr = CreateTextFormat(m_fontFamily, m_fontSize, m_fontWeight);
    if (FAILED(hr)) return false;

    // Create device-dependent resources (render target, brush)
    return CreateDeviceResources();
}

bool D2DRenderer::CreateDeviceResources() {
    if (!m_factory || !IsWindow(m_hwnd)) return false;

    // Release old resources
    DiscardDeviceResources();

    RECT rc{};
    ::GetClientRect(m_hwnd, &rc);

    D2D1_SIZE_U size = D2D1::SizeU(
        static_cast<UINT>(rc.right - rc.left),
        static_cast<UINT>(rc.bottom - rc.top));

    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    HRESULT hr;
    if(m_surface==Surface::BufferedLayered){
        // Draw offscreen. Only EndDraw publishes a complete frame;
        // clearing/rebinding/recovering the renderer never clears the desktop.
        rtProps.type=D2D1_RENDER_TARGET_TYPE_SOFTWARE;
        hr=m_factory->CreateDCRenderTarget(&rtProps,m_dcTarget.ReleaseAndGetAddressOf());
        if(FAILED(hr))return false;
        m_renderTarget=m_dcTarget;
        // Device recreation must rebind even when the retained DIB size matches.
        if(!ResizeLayeredBuffer(size.width,size.height)){DiscardDeviceResources();return false;}
        RECT bounds{0,0,static_cast<LONG>(size.width),static_cast<LONG>(size.height)};
        if(FAILED(m_dcTarget->BindDC(m_frameDC,&bounds))){DiscardDeviceResources();return false;}
    }else{
        const auto hwndProps=D2D1::HwndRenderTargetProperties(m_hwnd,size,D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS);
        hr=m_factory->CreateHwndRenderTarget(rtProps,hwndProps,m_windowTarget.ReleaseAndGetAddressOf());
        if(FAILED(hr))return false;
        m_renderTarget=m_windowTarget;
    }

    const UINT dpi=GetDpiForWindow(m_hwnd);
    m_renderTarget->SetDpi(static_cast<float>(dpi?dpi:96),static_cast<float>(dpi?dpi:96));

    m_surface_parent=GetParent(m_hwnd);
    // Set text antialiasing
    // Avoid coloured subpixel fringes on transparent/chroma-keyed overlays.
    m_renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

    // Create a reusable solid color brush
    hr = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White),
        m_brush.ReleaseAndGetAddressOf());
    if (FAILED(hr)){DiscardDeviceResources();return false;}

    return true;
}

HRESULT D2DRenderer::CreateTextFormat(
    const std::wstring& family, float size, DWRITE_FONT_WEIGHT weight) {
    if (!m_writeFactory) return E_FAIL;

    if (!std::isfinite(size) || size <= 0) return E_INVALIDARG;
    const auto key = std::make_pair(family, std::make_pair(size, static_cast<int>(weight)));
    if (const auto found = m_formats.find(key); found != m_formats.end()) {
        m_textFormat = found->second;
        return S_OK;
    }

    ComPtr<IDWriteTextFormat> candidate;
    HRESULT hr = m_writeFactory->CreateTextFormat(
        family.c_str(),
        nullptr,  // font collection (system default)
        weight,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        size,
        L"en-US",
        candidate.ReleaseAndGetAddressOf());
    if (FAILED(hr)) return hr;

    m_textFormat = std::move(candidate);
    // Default alignment: top-left
    m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    // Trim text instead of wrapping
    m_textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    if (m_formats.size() >= 64) m_formats.clear();
    m_formats[key] = m_textFormat;

    return S_OK;
}

void D2DRenderer::DiscardDeviceResources() {
    m_drawing=false;
    m_renderTarget.Reset();
    m_windowTarget.Reset();
    m_dcTarget.Reset();
    m_brush.Reset();
}

void D2DRenderer::ReleaseLayeredBuffer(){
    if(m_frameDC&&m_previousBitmap)SelectObject(m_frameDC,m_previousBitmap);
    if(m_frameBitmap)DeleteObject(m_frameBitmap);
    if(m_frameDC)DeleteDC(m_frameDC);
    m_frameDC=nullptr;m_frameBitmap=nullptr;m_previousBitmap=nullptr;
    m_frameWidth=m_frameHeight=0;
}

bool D2DRenderer::ResizeLayeredBuffer(UINT width,UINT height){
    if(!width||!height||!m_dcTarget)return false;
    if(m_frameDC&&width==m_frameWidth&&height==m_frameHeight)return true;
    BITMAPINFO info{};
    info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=static_cast<LONG>(width);
    info.bmiHeader.biHeight=-static_cast<LONG>(height);
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    HDC dc=CreateCompatibleDC(nullptr);
    if(!dc)return false;
    void* pixels=nullptr;
    HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
    if(!bitmap){DeleteDC(dc);return false;}
    const HGDIOBJ previous=SelectObject(dc,bitmap);
    if(!previous||previous==HGDI_ERROR){DeleteObject(bitmap);DeleteDC(dc);return false;}
    SetLayout(dc,LAYOUT_BITMAPORIENTATIONPRESERVED);
    RECT bounds{0,0,static_cast<LONG>(width),static_cast<LONG>(height)};
    if(FAILED(m_dcTarget->BindDC(dc,&bounds))){
        SelectObject(dc,previous);DeleteObject(bitmap);DeleteDC(dc);return false;
    }
    ReleaseLayeredBuffer();
    m_frameDC=dc;m_frameBitmap=bitmap;m_previousBitmap=previous;
    m_frameWidth=width;m_frameHeight=height;
    return true;
}

void D2DRenderer::Resize(UINT width, UINT height) {
    if(m_drawing||!width||!height)return;
    // A parent transition changes DWM's redirection surface. The HWND stays the
    // same, but its previous swap chain can keep presenting into the old surface.
    if(m_surface==Surface::Window&&m_surface_parent!=GetParent(m_hwnd))DiscardDeviceResources();
    if (!m_renderTarget) CreateDeviceResources();
    if (m_renderTarget) {
        UINT dpi = GetDpiForWindow(m_hwnd);
        m_renderTarget->SetDpi(static_cast<float>(dpi ? dpi : 96), static_cast<float>(dpi ? dpi : 96));
        if(m_surface==Surface::BufferedLayered){
            if(!ResizeLayeredBuffer(width,height))DiscardDeviceResources();
        }else{
            const auto size=m_windowTarget->GetPixelSize();
            // Resize invalidates the back buffer, even if the size is unchanged.
            if((size.width!=width||size.height!=height)&&FAILED(m_windowTarget->Resize(D2D1::SizeU(width,height))))
                DiscardDeviceResources();
        }
    }
}

bool D2DRenderer::BeginDraw(D2D1_COLOR_F clear_color) {
    if(m_drawing)return false;
    if (!m_renderTarget && !CreateDeviceResources()) return false;
    if (!m_renderTarget) return false;
    m_renderTarget->BeginDraw();
    m_drawing=true;
    if(m_surface==Surface::BufferedLayered)clear_color=D2D1::ColorF(12.0f/255,12.0f/255,12.0f/255,1);
    m_renderTarget->Clear(clear_color);
    return true;
}

HRESULT D2DRenderer::EndDraw() {
    if (!m_renderTarget||!m_drawing) return E_FAIL;
    m_drawing=false;

    HRESULT hr = m_renderTarget->EndDraw();
    if (FAILED(hr)) {
        // Keep the last committed image. Recreate resources on the next paint
        // after device loss or an invalid draw state; never present a failed frame.
        DiscardDeviceResources();
        return hr;
    }
    if(SUCCEEDED(hr)&&m_surface==Surface::BufferedLayered){
        RECT client{};
        if(!GetClientRect(m_hwnd,&client))return HRESULT_FROM_WIN32(GetLastError());
        // A layout update may resize the HWND during measurement. Keep its last
        // complete frame until a paint for the new size; never resize it backwards.
        if(client.right!=static_cast<LONG>(m_frameWidth)||client.bottom!=static_cast<LONG>(m_frameHeight)){
            InvalidateRect(m_hwnd,nullptr,FALSE);return S_FALSE;
        }
        HDC window_dc=GetDC(m_hwnd);
        if(!window_dc)return HRESULT_FROM_WIN32(GetLastError());
        // One full-buffer copy is the only operation that touches the visible
        // surface. In-progress clears/text drawing stay entirely offscreen.
        const BOOL copied=BitBlt(window_dc,0,0,m_frameWidth,m_frameHeight,m_frameDC,0,0,SRCCOPY);
        const DWORD error=copied?ERROR_SUCCESS:GetLastError();
        ReleaseDC(m_hwnd,window_dc);
        GdiFlush();
        if(!copied)return HRESULT_FROM_WIN32(error);
    }
    return hr;
}

void D2DRenderer::DrawText(const std::wstring& text, float x, float y, D2D1_COLOR_F color) {
    if (!m_renderTarget || !m_textFormat || !m_brush || text.empty()) return;
    float dx = 96, dy = 96;
    m_renderTarget->GetDpi(&dx, &dy);
    x = std::round(x * dx / 96.0f) * 96.0f / dx;
    y = std::round(y * dy / 96.0f) * 96.0f / dy;

    D2D1_SIZE_F targetSize = m_renderTarget->GetSize();
    D2D1_RECT_F rect = D2D1::RectF(x, y, x + 500.0f, targetSize.height);

    m_brush->SetColor(color);
    m_renderTarget->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        m_textFormat.Get(),
        rect,
        m_brush.Get(),
        D2D1_DRAW_TEXT_OPTIONS_NONE);
}

void D2DRenderer::DrawText(const std::wstring& text, D2D1_RECT_F rect, D2D1_COLOR_F color) {
    if (!m_renderTarget || !m_textFormat || !m_brush || text.empty()) return;

    m_brush->SetColor(color);
    m_renderTarget->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        m_textFormat.Get(),
        rect,
        m_brush.Get(),
        D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

void D2DRenderer::FillRoundedRect(D2D1_RECT_F rect, float radius, D2D1_COLOR_F color) {
    if (!m_renderTarget || !m_brush) return;

    m_brush->SetColor(color);
    D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(rect, radius, radius);
    m_renderTarget->FillRoundedRectangle(rr, m_brush.Get());
}

void D2DRenderer::FillGradientRoundedRect(D2D1_RECT_F rect, float radius, D2D1_COLOR_F top, D2D1_COLOR_F bottom) {
    if (!m_renderTarget) return;
    D2D1_GRADIENT_STOP stops[]={{0,top},{1,bottom}};
    ComPtr<ID2D1GradientStopCollection> collection;
    ComPtr<ID2D1LinearGradientBrush> brush;
    if (SUCCEEDED(m_renderTarget->CreateGradientStopCollection(stops,2,&collection)) &&
        SUCCEEDED(m_renderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(
            D2D1::Point2F(rect.left,rect.top),D2D1::Point2F(rect.left,rect.bottom)),collection.Get(),&brush)))
        m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(rect,radius,radius),brush.Get());
}

void D2DRenderer::DrawRoundedRect(D2D1_RECT_F rect, float radius, D2D1_COLOR_F color, float width) {
    if (!m_renderTarget || !m_brush) return;

    m_brush->SetColor(color);
    D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(rect, radius, radius);
    m_renderTarget->DrawRoundedRectangle(rr, m_brush.Get(), width);
}

void D2DRenderer::DrawLine(D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_COLOR_F color, float width) {
    if (!m_renderTarget || !m_brush) return;

    m_brush->SetColor(color);
    m_renderTarget->DrawLine(p1, p2, m_brush.Get(), width);
}

void D2DRenderer::SetFont(const std::wstring& family, float size, bool bold) {
    m_fontFamily = family;
    m_fontSize   = size;
    m_fontWeight = bold ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_REGULAR;
    CreateTextFormat(m_fontFamily, m_fontSize, m_fontWeight);
}

ID2D1RenderTarget* D2DRenderer::GetTarget() {
    return m_renderTarget.Get();
}

IDWriteFactory* D2DRenderer::GetWriteFactory() {
    return m_writeFactory.Get();
}

IDWriteTextFormat* D2DRenderer::GetTextFormat() {
    return m_textFormat.Get();
}

D2D1_SIZE_F D2DRenderer::MeasureText(const std::wstring& text) {
    D2D1_SIZE_F result = { 0.0f, 0.0f };

    if (!m_writeFactory || !m_textFormat || text.empty()) {
        return result;
    }

    ComPtr<IDWriteTextLayout> layout;
    HRESULT hr = m_writeFactory->CreateTextLayout(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        m_textFormat.Get(),
        10000.0f,  // max width (large enough)
        10000.0f,  // max height
        layout.GetAddressOf());

    if (FAILED(hr) || !layout) {
        return result;
    }

    DWRITE_TEXT_METRICS metrics{};
    hr = layout->GetMetrics(&metrics);
    if (SUCCEEDED(hr)) {
        result.width  = metrics.widthIncludingTrailingWhitespace;
        result.height = metrics.height;
    }

    return result;
}

} // namespace tmp
