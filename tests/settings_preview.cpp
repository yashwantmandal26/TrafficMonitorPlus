#include "windows/settings_window.h"

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    INITCOMMONCONTROLSEX common{sizeof(common),ICC_STANDARD_CLASSES};InitCommonControlsEx(&common);
    tmp::Settings settings=tmp::Settings::GetDefaults();
    tmp::SettingsWindow window;
    if(!window.Init(instance))return 1;
    window.live_settings=[&](){return settings;};
    window.on_settings_changed=[&](const tmp::Settings& value){settings=value;return true;};
    // Explicit preview fixture; never used by the production application.
    window.data_snapshot=[](){tmp::TelemetryData data;data.download_bytes_per_sec=7600000;data.upload_bytes_per_sec=320000;data.cpu_temp_celsius=48;data.cpu_usage_percent=23;data.gpu_usage_percent=7;data.gpu_prefix=L"iGPU:";return data;};
    window.usage_snapshot=[](){return std::make_pair(tmp::UsageTotal{2400000000,300000000},tmp::UsageTotal{12000000000,2300000000});};
    window.Show(settings);
    SetWindowTextW(window.GetHwnd(),L"Yash Monitor - Settings preview");
    MSG message{};
    while(window.IsVisible()&&GetMessageW(&message,nullptr,0,0)>0){TranslateMessage(&message);DispatchMessageW(&message);}
    CoUninitialize();return 0;
}
