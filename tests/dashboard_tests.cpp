#include "windows/flyout_window.h"
#include "rendering/dpi_helper.h"
#include <iostream>
#include <atomic>

namespace tmp {
struct FlyoutWindowTestAccess {
    static D2D1_RECT_F rect(const FlyoutWindow& w,int id) {
        for(const auto& c:w.m_controls)if(c.id==id)return c.rect;
        throw std::runtime_error("Missing control "+std::to_string(id));
    }
    static bool has(const FlyoutWindow& w,int id){for(const auto& c:w.m_controls)if(c.id==id)return true;return false;}
    static int page(const FlyoutWindow& w){return w.m_scroll;}
    static DWORD firstPid(const FlyoutWindow& w){return w.m_kill_actions.empty()?0:w.m_kill_actions.front().target.pid;}
    static size_t pendingCount(const FlyoutWindow& w){return w.m_pending_targets.size();}
    static void servicePicker(FlyoutWindow& w){w.m_confirm_active=true;w.m_service_picker=true;w.m_service_page=0;w.m_service_options={{L"svchost.exe",L"Service A",123,true,L"svcA"},{L"svchost.exe",L"Service B",123,true,L"svcB"}};InvalidateRect(w.GetHwnd(),nullptr,FALSE);}
    static bool fits(const FlyoutWindow& w){
        for(const auto& c:w.m_controls)if(c.rect.left<0||c.rect.top<0||c.rect.right>w.GetWidth()||c.rect.bottom>w.GetHeight())return false;
        for(size_t i=0;i<w.m_controls.size();++i)for(size_t j=i+1;j<w.m_controls.size();++j){
            const auto& a=w.m_controls[i].rect;const auto& b=w.m_controls[j].rect;
            if((std::max)(a.left,b.left)<(std::min)(a.right,b.right)&&(std::max)(a.top,b.top)<(std::min)(a.bottom,b.bottom))return false;
        }
        return true;
    }
};
}

int main(int argc,char** argv){
    if(argc>1&&std::string(argv[1])=="--owned-child"){Sleep(30000);return 0;}
    tmp::DpiHelper::Init();CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    int failures=0;
    auto check=[&](bool pass,const char* name){if(!pass){std::cerr<<name<<"\n";++failures;}};
    auto pump=[](ULONGLONG duration){const auto until=GetTickCount64()+duration;while(GetTickCount64()<until){MsgWaitForMultipleObjects(0,nullptr,FALSE,10,QS_ALLINPUT);MSG m{};while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}}};
    {
        tmp::FlyoutWindow w;check(w.Init(GetModuleHandleW(nullptr)),"init");
        int copies=0,usage=0,tasks=0,settings=0,opens=0,refresh=0,callbacks=0;
        wchar_t opened=0;std::atomic<int> reads{0};
        w.on_copy_ip=[&](){++copies;return true;};w.on_data_usage=[&](){++usage;};
        w.on_open_task_manager=[&](){++tasks;};w.on_open_settings=[&](){++settings;};
        w.on_open_drive=[&](wchar_t c){opened=c;++opens;};
        w.on_kill_target=[&](const std::wstring&,DWORD,bool){++callbacks;return true;};
        tmp::NetworkUsageReport report{};
        w.network_usage_provider=[&](bool force){if(force)++refresh;return report;};
        w.storage_provider=[&](){++reads;std::vector<tmp::DiskInfo> disks;for(int i=0;i<7;++i){tmp::DiskInfo d{};d.letter=L'C'+i;d.label=L"Long volume label for clipping";d.total=1000000000;d.free=100000000;d.used_pct=90;disks.push_back(d);}return disks;};
        tmp::TelemetryData data{};data.cpu_usage_percent=32;data.ram_usage_percent=50;data.timestamp=std::chrono::steady_clock::now();
        std::vector<tmp::ProcessInfo> processes;
        for(int i=0;i<19;++i){tmp::ProcessInfo p{};p.pid=100+i;p.name=L"Process.exe";p.cpu_percent=19-i;p.memory_bytes=i*1000000;processes.push_back(p);}
        w.UpdateData(data,processes,L"192.168.1.2");pump(40);check(reads==0,"hidden updates must not read storage");
        w.Show({500,800},true);UpdateWindow(w.GetHwnd());
        auto click=[&](int id){auto b=tmp::FlyoutWindowTestAccess::rect(w,id);RECT r{};GetClientRect(w.GetHwnd(),&r);const float s=(std::min)(r.right/float(w.GetWidth()),r.bottom/float(w.GetHeight()));SendMessageW(w.GetHwnd(),WM_LBUTTONUP,0,MAKELPARAM(int((b.left+b.right)*s/2),int((b.top+b.bottom)*s/2)));UpdateWindow(w.GetHwnd());check(tmp::FlyoutWindowTestAccess::fits(w),"controls must fit without overlap");};
        wchar_t title[128]{};GetWindowTextW(w.GetHwnd(),title,128);check(std::wstring(title)==L"Quick Panel","exact Quick Panel title");
        const int emptyHeight=w.GetHeight();
        RECT stableBounds{};GetWindowRect(w.GetHwnd(),&stableBounds);
        click(0);click(1);click(2);click(8);check(copies==1&&usage==1&&tasks==1&&settings==1,"footer callbacks");
        click(15);check(refresh==1,"network retry callback");
        report.status=tmp::ReportStatus::Complete;report.ready=true;
        for(int i=0;i<5;++i){tmp::AppUsageEntry e{};e.display_name=L"Historical app";e.download=(i+1)*1000000;e.upload=(5-i)*100000;report.items.push_back(e);}
        w.UpdateNetworkReport(report);UpdateWindow(w.GetHwnd());
        check(w.GetHeight()==emptyHeight,"populated network content keeps fixed height");check(!tmp::FlyoutWindowTestAccess::has(w,200),"historical usage must not expose unverified termination");
        check(tmp::FlyoutWindowTestAccess::fits(w),"full network controls fit");
        report.items.clear();report.status=tmp::ReportStatus::Failed;w.UpdateNetworkReport(report);UpdateWindow(w.GetHwnd());
        check(w.GetHeight()==emptyHeight,"failed/empty content keeps fixed height");click(15);check(refresh==2,"failed network report can retry");
        tmp::LiveNetworkSnapshot live{};live.state=tmp::LiveNetworkState::Live;live.sampled_at=std::chrono::steady_clock::now();
        live.entries.push_back({L"test.exe",L"test.exe",240000,80000,4800000,1600000});w.UpdateLiveNetwork(live);UpdateWindow(w.GetHwnd());
        // Every tab/filter, network state and page preserves physical bounds and footer position.
        const auto fixedFooter=tmp::FlyoutWindowTestAccess::rect(w,0);
        for(int id:{21,22,20,5,6,4}){
            click(id);RECT current{};GetWindowRect(w.GetHwnd(),&current);
            check(EqualRect(&stableBounds,&current),"option selection must not resize or move panel");
            const auto footer=tmp::FlyoutWindowTestAccess::rect(w,0);
            check(footer.top==fixedFooter.top&&footer.left==fixedFooter.left,"footer never moves between tabs");
        }
        pump(150);UpdateWindow(w.GetHwnd());
        const int storage_before_open=reads;
        click(7);SendMessageW(w.GetHwnd(),WM_ACTIVATE,WA_INACTIVE,0);check(w.IsVisible(),"pin keeps panel visible");
        // Overview wheel scroll moves content, never switches the selected filter.
        SendMessageW(w.GetHwnd(),WM_MOUSEWHEEL,MAKEWPARAM(0,-WHEEL_DELTA),0);UpdateWindow(w.GetHwnd());
        check(tmp::FlyoutWindowTestAccess::page(w)>0,"overview has real scrolling");
        click(22);check(tmp::FlyoutWindowTestAccess::firstPid(w)==100,"CPU ranking");
        click(200);check(w.IsConfirmingKill(),"close opens modal");
        check(!tmp::FlyoutWindowTestAccess::has(w,0),"modal removes background targets");
        SendMessageW(w.GetHwnd(),WM_KEYDOWN,VK_RETURN,0);UpdateWindow(w.GetHwnd());check(!w.IsConfirmingKill(),"Enter defaults to Cancel");
        click(200);click(51);
        click(5);click(10);check(tmp::FlyoutWindowTestAccess::firstPid(w)==118,"memory sort uses all processes");
        const int topOffset=tmp::FlyoutWindowTestAccess::page(w);
        SendMessageW(w.GetHwnd(),WM_MOUSEWHEEL,MAKEWPARAM(0,-WHEEL_DELTA),0);UpdateWindow(w.GetHwnd());
        check(tmp::FlyoutWindowTestAccess::page(w)>topOffset,"process wheel scrolls pixels");
        // Drag the visible scrollbar to its bottom, including beyond the window.
        RECT client{};GetClientRect(w.GetHwnd(),&client);const float sc=client.right/float(w.GetWidth());
        SendMessageW(w.GetHwnd(),WM_LBUTTONDOWN,0,MAKELPARAM(int(466*sc),int(250*sc)));
        SendMessageW(w.GetHwnd(),WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(int(466*sc),int(800*sc)));
        SendMessageW(w.GetHwnd(),WM_LBUTTONUP,0,MAKELPARAM(int(466*sc),int(800*sc)));UpdateWindow(w.GetHwnd());
        const int bottom=tmp::FlyoutWindowTestAccess::page(w);
        SendMessageW(w.GetHwnd(),WM_KEYDOWN,VK_END,0);UpdateWindow(w.GetHwnd());
        check(bottom==tmp::FlyoutWindowTestAccess::page(w)&&bottom>64,"thumb drag reaches exact end");
        check(tmp::FlyoutWindowTestAccess::fits(w),"scrolled controls are clipped to viewport");
        SendMessageW(w.GetHwnd(),WM_KEYDOWN,VK_HOME,0);UpdateWindow(w.GetHwnd());check(tmp::FlyoutWindowTestAccess::page(w)==0,"Home returns to top");
        click(6);pump(150);UpdateWindow(w.GetHwnd());check(reads==storage_before_open+1,"storage opens asynchronously");
        click(102);check(opened==L'C',"open first drive");
        SendMessageW(w.GetHwnd(),WM_KEYDOWN,VK_END,0);UpdateWindow(w.GetHwnd());click(108);check(opened==L'I',"last drive reachable by scrolling");
        SendMessageW(w.GetHwnd(),WM_KEYDOWN,VK_HOME,0);UpdateWindow(w.GetHwnd());click(13);pump(150);UpdateWindow(w.GetHwnd());check(reads==storage_before_open+2,"explicit storage refresh");
        const int before=reads;for(int i=0;i<10;++i)SendMessageW(w.GetHwnd(),WM_PAINT,0,0);check(reads==before,"paint never queries drives");
        pump(5100);w.UpdateData(data,processes);pump(150);UpdateWindow(w.GetHwnd());check(reads==before+1,"visible storage refresh");
        click(5);SendMessageW(w.GetHwnd(),WM_KEYDOWN,VK_END,0);w.UpdateData(data,{});UpdateWindow(w.GetHwnd());check(tmp::FlyoutWindowTestAccess::page(w)==0,"shrinking list clamps offset");
        std::wstring feedback;
        check(!w.TerminateTarget({L"missing.exe",L"Missing",MAXDWORD,false,L""},feedback),"nonexistent PID reports failure");
        check(!w.TerminateTarget({L"Process.exe",L"Unknown",0,false,L""},feedback),"no name-based process termination");
        check(!w.TerminateTarget({L"System",L"System",4,false,L""},feedback),"system protected");
        check(!w.TerminateTarget({L"service",L"Service",0,true,L""},feedback),"unverified service protected");check(callbacks==0,"failed termination sends no success callback");
        // Verify service selection without stopping any real Windows service.
        tmp::FlyoutWindowTestAccess::servicePicker(w);UpdateWindow(w.GetHwnd());click(61);
        check(w.GetPendingKill().is_service&&w.GetPendingKill().service_name==L"svcB"&&tmp::FlyoutWindowTestAccess::pendingCount(w)==1,"shared-host picker selects only the requested service");click(51);
        check(!w.TerminateTarget({L"svchost.exe",L"Mismatch",MAXDWORD,true,L"Winmgmt",1,L"C:\\Windows\\System32\\svchost.exe"},feedback),"service PID mismatch cannot stop a real service");
        // Only close children created by this test, never any user application.
        wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);
        PROCESS_INFORMATION child{},sibling{};STARTUPINFOW start{sizeof(start)};
        auto spawn=[&](PROCESS_INFORMATION& p){std::wstring command=L"\""+std::wstring(exe)+L"\" --owned-child";return CreateProcessW(exe,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&start,&p)!=FALSE;};
        const bool first=spawn(child),second=spawn(sibling);check(first&&second,"create owned app processes");
        if(first&&second){
            FILETIME birth{},exit{},kernel{},user{};GetProcessTimes(child.hProcess,&birth,&exit,&kernel,&user);
            const uint64_t created=(uint64_t(birth.dwHighDateTime)<<32)|birth.dwLowDateTime;
            check(!w.TerminateTarget({fs::path(exe).filename().wstring(),L"Mismatch",child.dwProcessId,false,L"",created+1,exe},feedback),"creation time mismatch rejects a reused PID");
            click(4);click(21);
            live.entries.clear();tmp::LiveNetworkEntry row{};row.id=exe;row.name=fs::path(exe).filename().wstring();row.upload_bps=1000;row.download_bps=2000;row.processes.push_back({child.dwProcessId,created});live.entries.push_back(row);live.sampled_at=std::chrono::steady_clock::now();w.UpdateLiveNetwork(live);UpdateWindow(w.GetHwnd());
            check(tmp::FlyoutWindowTestAccess::has(w,200)&&tmp::FlyoutWindowTestAccess::has(w,201),"upload and download rows both have close buttons");
            click(200);check(w.IsConfirmingKill()&&tmp::FlyoutWindowTestAccess::pendingCount(w)==2,"upload cross freezes the exact app process group");click(51);
            check(WaitForSingleObject(child.hProcess,0)==WAIT_TIMEOUT&&WaitForSingleObject(sibling.hProcess,0)==WAIT_TIMEOUT,"cancel leaves both app processes running");
            click(201);check(w.IsConfirmingKill(),"download cross opens confirmation");click(50);
            check(WaitForSingleObject(child.hProcess,3000)==WAIT_OBJECT_0&&WaitForSingleObject(sibling.hProcess,3000)==WAIT_OBJECT_0,"confirmed network close terminates the owned app group");
            check(callbacks==2,"success callbacks only for successfully closed processes");
            click(200);check(!w.IsConfirmingKill(),"stale network row cannot target an exited or reused process");
        }
        for(auto* childInfo:{&child,&sibling})if(childInfo->hProcess){CloseHandle(childInfo->hThread);CloseHandle(childInfo->hProcess);}
        click(7);SendMessageW(w.GetHwnd(),WM_ACTIVATE,WA_INACTIVE,0);check(!w.IsVisible(),"unpin restores dismissal");
        w.Show({500,800},true);UpdateWindow(w.GetHwnd());SendMessageW(w.GetHwnd(),WM_KEYDOWN,VK_TAB,0);SendMessageW(w.GetHwnd(),WM_KEYDOWN,VK_TAB,0);SendMessageW(w.GetHwnd(),WM_KEYDOWN,VK_RETURN,0);check(!w.IsVisible(),"keyboard close");
    }
    CoUninitialize();if(failures)std::cerr<<failures<<" dashboard failures\n";else std::cout<<"Quick Panel layout, navigation, data states, storage and owned-child termination passed\n";
    return failures?1:0;
}
