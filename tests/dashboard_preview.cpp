// Development-only visual fixture. No simulated readings enter the production app.
#include "windows/flyout_window.h"
#include "rendering/dpi_helper.h"

namespace {
tmp::FlyoutWindow* preview=nullptr;
bool empty=false,failed=false;
void Tick(){
    tmp::TelemetryData data{};
    const double t=GetTickCount64()/1400.0;
    data.download_bytes_per_sec=2800000+1700000*std::sin(t);
    data.upload_bytes_per_sec=420000+280000*std::sin(t*.7);
    data.total_session_download=3200000000;data.total_session_upload=480000000;
    data.cpu_usage_percent=23+12*std::sin(t*.3);data.cpu_temp_celsius=48;data.cpu_freq_ghz=3.6;
    data.ram_usage_percent=52;data.ram_used_bytes=8ULL*1024*1024*1024;
    data.ram_total_bytes=16ULL*1024*1024*1024;
    data.gpu_usage_percent=12;data.gpu_temp_celsius=43;data.gpu_prefix=L"iGPU:";
    data.timestamp=std::chrono::steady_clock::now();
    std::vector<tmp::ProcessInfo> processes;
    const wchar_t* names[]={L"Antigravity.exe",L"msedge.exe",L"TrafficMonitorPlus.exe",L"WindowsTerminal.exe",L"explorer.exe",L"ApplicationFrameHost.exe",L"SearchHost.exe",L"RuntimeBroker.exe"};
    for(int i=0;i<8;++i){tmp::ProcessInfo p{};p.pid=1200+i*104;p.name=names[i];p.cpu_percent=12.0/(i+1);p.memory_bytes=(80ULL+(i*137)%700)*1024*1024;processes.push_back(p);}
    preview->UpdateData(data,processes,L"192.168.29.156");
    tmp::LiveNetworkSnapshot live{};live.state=failed?tmp::LiveNetworkState::Unavailable:tmp::LiveNetworkState::Live;
    live.message=failed?L"Live tracking unavailable (test fixture)":L"Live app traffic \u00B7 all adapters";
    live.coverage_start=L"01:58";live.sampled_at=std::chrono::steady_clock::now();
    if(!empty&&!failed)for(int i=0;i<5;++i){tmp::LiveNetworkEntry e{};e.id=names[i];e.name=names[i];e.download_bps=data.download_bytes_per_sec/(i+1);e.upload_bps=data.upload_bytes_per_sec/(5-i);e.today_download=(5-i)*123456789ULL;e.today_upload=(i+1)*2345678ULL;live.entries.push_back(e);}
    preview->UpdateLiveNetwork(live);
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR args,int){
    empty=std::wstring(args).find(L"--empty")!=std::wstring::npos;failed=std::wstring(args).find(L"--failed")!=std::wstring::npos;
    tmp::DpiHelper::Init();CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    {
        tmp::FlyoutWindow window;preview=&window;
        if(!window.Init(instance))return 1;
        // Give the development preview its own discoverable application window.
        SetWindowLongPtrW(window.GetHwnd(),GWL_EXSTYLE,WS_EX_APPWINDOW|WS_EX_TOPMOST);
        window.SetPinned(true);
        window.storage_provider=[](){
            std::vector<tmp::DiskInfo> drives;
            const wchar_t* labels[]={L"Windows",L"Projects & media",L"Archive",L"External backup",L"Scratch"};
            for(int i=0;i<5;++i){tmp::DiskInfo d{};d.letter=L'C'+i;d.label=labels[i];d.total=512ULL*1024*1024*1024;d.used_pct=46+i*12;d.free=static_cast<uint64_t>(d.total*(100-d.used_pct)/100);d.used=d.total-d.free;drives.push_back(d);}
            return drives;
        };
        window.on_copy_ip=[](){return true;};
        tmp::NetworkUsageReport report{};
        report.status=tmp::ReportStatus::Complete;report.ready=true;
        if (std::wstring(args).find(L"--empty")==std::wstring::npos) {
            for(int i=0;i<5;++i){tmp::AppUsageEntry e{};e.display_name=i==0?L"Microsoft Edge":i==1?L"Windows Update":i==2?L"Codex":i==3?L"Service Host":L"Cloud sync";e.category=i%2?tmp::AppUsageCategory::Service:tmp::AppUsageCategory::App;e.download=(5-i)*123456789ULL;e.upload=(i+1)*2345678ULL;report.items.push_back(e);}
        }
        if(std::wstring(args).find(L"--failed")!=std::wstring::npos){report.items.clear();report.status=tmp::ReportStatus::Failed;}
        window.UpdateNetworkReport(report);
        window.RefreshStorageSnapshot();Tick();
        RECT work{};SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
        window.Show({(work.left+work.right)/2,work.bottom},true);
        SetWindowTextW(window.GetHwnd(),L"Quick Panel - preview (sample data)");
        SetTimer(window.GetHwnd(),91,700,[](HWND,UINT,UINT_PTR,DWORD){Tick();});
        MSG message{};while(window.IsVisible()&&GetMessageW(&message,nullptr,0,0)>0){TranslateMessage(&message);DispatchMessageW(&message);}
        KillTimer(window.GetHwnd(),91);
    }
    CoUninitialize();return 0;
}
