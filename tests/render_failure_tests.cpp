#include "pch.h"
#include "rendering/d2d_renderer.h"
#include <cassert>
#include <iostream>

using namespace tmp;

// Helper to register and create a hidden test window
HWND CreateTestWindow() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"RenderFailureTestWnd";
    RegisterClassExW(&wc);

    return CreateWindowExW(
        0, wc.lpszClassName, L"Test", WS_POPUP,
        0, 0, 400, 300, nullptr, nullptr, wc.hInstance, nullptr
    );
}

void TestInvalidWindowAllocationFailure() {
    std::cout << "[Test 1] Allocation failure with invalid HWND\n";
    D2DRenderer renderer;
    // Attempt init with nullptr HWND
    bool init_null = renderer.Init(nullptr);
    assert(!init_null);
    assert(!renderer.HasValidTarget());
    assert(renderer.GetTarget() == nullptr);

    // Frame startup must return false and not crash
    bool safe_draw = renderer.BeginDraw();
    assert(!safe_draw);
    assert(renderer.GetTarget() == nullptr);

    // EndDraw on uninitialized target must fail safely without crash
    HRESULT hr = renderer.EndDraw();
    assert(FAILED(hr));
    std::cout << "  Passed. Handled invalid HWND without crashing or dereferencing null.\n";
}

void TestValidWindowAndDeviceLossRecovery() {
    std::cout << "[Test 2] Valid window lifecycle and simulated device-loss recovery\n";
    HWND hwnd = CreateTestWindow();
    assert(hwnd != nullptr);

    D2DRenderer renderer;
    bool init_ok = renderer.Init(hwnd);
    assert(init_ok);

    // Frame 1: Normal drawing
    bool draw1 = renderer.BeginDraw(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    assert(draw1);
    assert(renderer.HasValidTarget());
    assert(renderer.GetTarget() != nullptr);
    renderer.DrawText(L"Test", 10.0f, 10.0f, D2D1::ColorF(D2D1::ColorF::White));
    HRESULT hr1 = renderer.EndDraw();
    assert(SUCCEEDED(hr1));
    std::cout << "  Frame 1 normal render: OK.\n";

    // Simulate device loss: discard resources
    std::cout << "  Simulating device loss...\n";
    renderer.DiscardDeviceResources();
    assert(!renderer.HasValidTarget());
    assert(renderer.GetTarget() == nullptr);

    // Simulated window paint handler during device loss
    auto SafePaintHandler = [&renderer]() -> bool {
        if (!renderer.BeginDraw()) {
            // Must safely skip without dereferencing GetTarget()
            return false;
        }
        auto* target = renderer.GetTarget();
        if (!target) return false;
        float dx = 96.0f, dy = 96.0f;
        target->GetDpi(&dx, &dy);
        renderer.EndDraw();
        return true;
    };

    // Frame 2: BeginDraw should recreate resources and recover cleanly
    bool recovered = SafePaintHandler();
    assert(recovered == true);
    assert(renderer.HasValidTarget());
    std::cout << "  Frame 2 recovered successfully.\n";

    HWND parent=CreateTestWindow();
    ComPtr<ID2D1RenderTarget> before=renderer.GetTarget();
    SetWindowLongPtrW(hwnd,GWL_STYLE,WS_CHILD);
    SetParent(hwnd,parent);
    renderer.Resize(400,300);
    if(renderer.GetTarget()==before.Get()||!renderer.BeginDraw())throw std::runtime_error("Reparenting retained a stale graphics target");
    if(FAILED(renderer.EndDraw()))throw std::runtime_error("Embedded rendering failed");
    before=renderer.GetTarget();
    SetWindowLongPtrW(hwnd,GWL_STYLE,WS_POPUP);
    SetParent(hwnd,nullptr);
    renderer.Resize(400,300);
    if(renderer.GetTarget()==before.Get()||!renderer.BeginDraw())throw std::runtime_error("Detaching retained a stale graphics target");
    if(FAILED(renderer.EndDraw()))throw std::runtime_error("Detached rendering failed");
    DestroyWindow(parent);

    DestroyWindow(hwnd);
    std::cout << "  Passed. Recovery succeeded without busy retry loops.\n";
}

void TestLayeredFramePersistence(){
    std::cout<<"[Test 3] Complete layered frames persist during repaint, resize and device recovery\n";
    HWND parent=CreateTestWindow();
    SetWindowPos(parent,HWND_TOPMOST,80,100,260,120,SWP_SHOWWINDOW|SWP_NOACTIVATE);
    HWND window=CreateWindowExW(WS_EX_LAYERED|WS_EX_NOACTIVATE,L"RenderFailureTestWnd",L"Layered persistence test",
        WS_CHILD|WS_VISIBLE,0,0,180,70,parent,nullptr,GetModuleHandleW(nullptr),nullptr);
    auto require=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    require(window!=nullptr,"Could not create a layered child window (Windows compatibility manifest required)");
    {
        D2DRenderer renderer;
        require(renderer.Init(window,D2DRenderer::Surface::BufferedLayered),"Layered renderer initialization failed");
        auto present=[&](){
            require(renderer.BeginDraw(),"Layered BeginDraw failed");
            renderer.FillRoundedRect(D2D1::RectF(0,0,80,40),0,D2D1::ColorF(D2D1::ColorF::White));
            require(renderer.EndDraw()==S_OK,"Layered frame was not committed");
        };
        int visibility_check=0;
        auto visible=[&](){
            ++visibility_check;
            DwmFlush();
            RECT rect{};GetWindowRect(window,&rect);
            HDC screen=GetDC(nullptr);
            const COLORREF pixel=GetPixel(screen,rect.left+20,rect.top+20);
            ReleaseDC(nullptr,screen);
            if(pixel==CLR_INVALID||GetRValue(pixel)<=245||GetGValue(pixel)<=245||GetBValue(pixel)<=245)
                std::cerr<<"Layered visibility check "<<visibility_check<<": pixel="<<unsigned(GetRValue(pixel))<<","<<unsigned(GetGValue(pixel))<<","<<unsigned(GetBValue(pixel))<<" at "<<rect.left+20<<","<<rect.top+20<<"\n";
            require(pixel!=CLR_INVALID&&GetRValue(pixel)>245&&GetGValue(pixel)>245&&GetBValue(pixel)>245,
                "Completed frame disappeared between paints");
        };
        present();visible();
        // Match the real taskbar: a nearly transparent sibling catches clicks
        // above the visual surface. It must not clip away the buffered image.
        HWND input=CreateWindowExW(WS_EX_LAYERED|WS_EX_NOACTIVATE,L"STATIC",L"Input fixture",
            WS_CHILD|WS_VISIBLE,0,0,180,70,parent,nullptr,GetModuleHandleW(nullptr),nullptr);
        require(input&&SetLayeredWindowAttributes(input,0,1,LWA_ALPHA),"Input fixture initialization failed");
        HDC input_dc=GetDC(input);RECT input_bounds{0,0,180,70};
        FillRect(input_dc,&input_bounds,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));ReleaseDC(input,input_dc);
        visible();
        const DWORD gdi_before=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
        for(int frame=0;frame<60;++frame){
            renderer.Resize(180,70); // Production calls this on every paint.
            require(renderer.BeginDraw(),"Repeated layered BeginDraw failed");
            visible(); // Offscreen buffer is now empty; desktop must still show the last frame.
            require(!renderer.BeginDraw(),"Nested drawing was accepted");
            renderer.FillRoundedRect(D2D1::RectF(0,0,80,40),0,D2D1::ColorF(D2D1::ColorF::White));
            require(renderer.EndDraw()==S_OK,"Repeated layered presentation failed");
            visible();
        }
        require(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)<=gdi_before+2,"Repeated paints leaked GDI resources");
        DestroyWindow(input);
        renderer.DiscardDeviceResources();visible();
        present();visible();
        SetWindowPos(window,nullptr,0,0,210,80,SWP_NOACTIVATE|SWP_NOZORDER);
        renderer.Resize(210,80);visible();present();visible();
        // A geometry change while a frame is being built must not publish an
        // empty/wrong-sized frame or revert the HWND to its previous dimensions.
        require(renderer.BeginDraw(),"Resize-race BeginDraw failed");
        SetWindowPos(window,nullptr,0,0,220,80,SWP_NOACTIVATE|SWP_NOZORDER);
        require(renderer.EndDraw()==S_FALSE,"Wrong-sized frame was published");
        RECT client{};GetClientRect(window,&client);
        require(client.right==220,"Presentation reverted the window size");visible();
        renderer.Resize(220,80);present();visible();
        ComPtr<ID2D1RenderTarget> before=renderer.GetTarget();
        SetWindowLongPtrW(window,GWL_STYLE,WS_POPUP|WS_VISIBLE);
        SetParent(window,nullptr);
        SetWindowPos(window,HWND_TOPMOST,80,100,220,80,SWP_NOACTIVATE);
        renderer.Resize(220,80);
        require(renderer.GetTarget()==before.Get(),"Reparenting needlessly recreated an offscreen renderer");
        present();visible();
    }
    DestroyWindow(window);DestroyWindow(parent);
    std::cout<<"  Passed: no blank frames across 60 repaints, resize and recovery.\n";
}

int main() {
    std::cout<<std::unitbuf;
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CoInitialize(nullptr);
    try{
        TestInvalidWindowAllocationFailure();
        TestValidWindowAndDeviceLossRecovery();
        TestLayeredFramePersistence();
    }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";CoUninitialize();return 1;}
    CoUninitialize();
    std::cout << "\nAll render failure and device-loss tests passed successfully!\n";
    return 0;
}
