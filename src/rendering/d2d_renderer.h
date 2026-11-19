// TrafficMonitorPlus - Direct2D Renderer
#pragma once

namespace tmp {

/// Owns the Direct2D factory, render target, and DirectWrite resources.
/// Provides drawing primitives for text, shapes, and lines.
class D2DRenderer {
public:
    D2DRenderer() = default;
    ~D2DRenderer();
    enum class Surface { Window, BufferedLayered };

    // Non-copyable, non-movable (COM pointers are tied to this instance)
    D2DRenderer(const D2DRenderer&) = delete;
    D2DRenderer& operator=(const D2DRenderer&) = delete;

    /// Initialize Direct2D and DirectWrite for the given window.
    /// Returns true on success.
    bool Init(HWND hwnd, Surface surface = Surface::Window);
    bool IsBoundTo(HWND hwnd) const { return m_hwnd == hwnd; }

    /// Resize the render target to the new window dimensions.
    void Resize(UINT width, UINT height);

    /// Begin a draw cycle. Clears with the specified background (default: transparent).
    /// Returns true if drawing started successfully; false if the render target is unavailable.
    bool BeginDraw(D2D1_COLOR_F clear_color = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

    /// End the draw cycle. Handles D2DERR_RECREATE_TARGET.
    /// Returns S_OK on success, or error HRESULT.
    HRESULT EndDraw();

    /// Check if a valid render target currently exists.
    bool HasValidTarget() const { return m_renderTarget != nullptr; }

    /// Discard device-dependent resources (render target and brush).
    void DiscardDeviceResources();

    /// Draw text at a specific position.
    void DrawText(const std::wstring& text, float x, float y, D2D1_COLOR_F color);

    /// Draw text within a rectangle.
    void DrawText(const std::wstring& text, D2D1_RECT_F rect, D2D1_COLOR_F color);

    /// Fill a rounded rectangle.
    void FillRoundedRect(D2D1_RECT_F rect, float radius, D2D1_COLOR_F color);
    void FillGradientRoundedRect(D2D1_RECT_F rect, float radius, D2D1_COLOR_F top, D2D1_COLOR_F bottom);

    /// Draw the outline of a rounded rectangle.
    void DrawRoundedRect(D2D1_RECT_F rect, float radius, D2D1_COLOR_F color, float width = 1.0f);

    /// Draw a line between two points.
    void DrawLine(D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_COLOR_F color, float width = 1.0f);

    /// Change the font used for text rendering.
    void SetFont(const std::wstring& family, float size, bool bold);
    const std::wstring& FontFamily() const { return m_fontFamily; }
    float FontSize() const { return m_fontSize; }
    bool FontBold() const { return m_fontWeight >= DWRITE_FONT_WEIGHT_SEMI_BOLD; }

    /// Raw access to the render target for advanced drawing.
    ID2D1RenderTarget* GetTarget();

    /// Raw access to the DirectWrite factory.
    IDWriteFactory* GetWriteFactory();

    /// Raw access to the current text format.
    IDWriteTextFormat* GetTextFormat();

    /// Measure the bounding box of a text string using the current text format.
    D2D1_SIZE_F MeasureText(const std::wstring& text);

private:
    /// Creates (or recreates) all device-dependent resources.
    bool CreateDeviceResources();
    bool ResizeLayeredBuffer(UINT width, UINT height);
    void ReleaseLayeredBuffer();

    /// Creates the default text format.
    HRESULT CreateTextFormat(const std::wstring& family, float size, DWRITE_FONT_WEIGHT weight);

    HWND m_hwnd = nullptr;
    HWND m_surface_parent = nullptr;
    Surface m_surface = Surface::Window;
    bool m_drawing = false;
    HDC m_frameDC = nullptr;
    HBITMAP m_frameBitmap = nullptr;
    HGDIOBJ m_previousBitmap = nullptr;
    UINT m_frameWidth = 0, m_frameHeight = 0;

    // Direct2D
    ComPtr<ID2D1Factory1>          m_factory;
    ComPtr<ID2D1RenderTarget>      m_renderTarget;
    ComPtr<ID2D1HwndRenderTarget>  m_windowTarget;
    ComPtr<ID2D1DCRenderTarget>    m_dcTarget;

    // DirectWrite
    ComPtr<IDWriteFactory>         m_writeFactory;
    ComPtr<IDWriteTextFormat>      m_textFormat;
    std::map<std::pair<std::wstring, std::pair<float, int>>, ComPtr<IDWriteTextFormat>> m_formats;

    // Reusable solid color brush
    ComPtr<ID2D1SolidColorBrush>   m_brush;

    // Current font settings (for recreation)
    std::wstring m_fontFamily = L"Segoe UI Variable Display";
    float        m_fontSize   = 9.0f;
    DWRITE_FONT_WEIGHT m_fontWeight = DWRITE_FONT_WEIGHT_SEMI_BOLD;
};

} // namespace tmp
