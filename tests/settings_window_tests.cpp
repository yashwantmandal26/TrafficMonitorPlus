#include "windows/settings_window.h"
#include <iostream>

int main(){
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    int failures=0;
    {
        tmp::SettingsWindow window;
        auto settings=tmp::Settings::GetDefaults();
        if(!window.Init(GetModuleHandleW(nullptr)))return 1;
        bool accept=true;int saves=0;
        window.live_settings=[&](){return settings;};
        window.on_settings_changed=[&](const tmp::Settings& value){++saves;if(accept)settings=value;return accept;};
        window.Show(settings);UpdateWindow(window.GetHwnd());
        auto click=[&](int x,int y){
            RECT rect{};GetClientRect(window.GetHwnd(),&rect);
            const float scale=(std::min)(rect.right/820.0f,rect.bottom/600.0f);
            SendMessageW(window.GetHwnd(),WM_LBUTTONUP,0,MAKELPARAM(static_cast<int>(x*scale),static_cast<int>(y*scale)));
            UpdateWindow(window.GetHwnd());
        };
        click(90,203); // Layout & position
        click(650,142); // Position & spacing
        const int values[]={-35,-20,7};
        for(int i=0;i<3;++i){
            HWND slider=GetDlgItem(window.GetHwnd(),1100+i);
            if(!slider||!IsWindowVisible(slider)){std::cerr<<"Missing visible position slider\n";++failures;continue;}
            SendMessageW(slider,TBM_SETPOS,TRUE,values[i]);
            SendMessageW(window.GetHwnd(),WM_HSCROLL,TB_THUMBPOSITION,reinterpret_cast<LPARAM>(slider));
        }
        window.Show(tmp::Settings::GetDefaults()); // Reopening must not reset an open draft.
        accept=false;click(735,565);
        if(!window.IsVisible()||settings.taskbar_offset_x==-35)++failures;
        accept=true;click(735,565);
        if(!window.IsVisible()||saves!=2||settings.taskbar_offset_x!=-35||settings.taskbar_offset_y!=-20||settings.item_spacing!=7){
            std::cerr<<"Apply did not persist slider values while keeping Settings open\n";++failures;
        }
        click(90,412); // Performance & sensors
        for(int i=0;i<3;++i)if(IsWindowVisible(GetDlgItem(window.GetHwnd(),1100+i)))++failures;
        click(385,202); // 0.7 sec
        click(735,565);
        if(!window.IsVisible()||settings.poll_interval_ms!=700)++failures;

        // Moved settings remain editable and share one draft across categories.
        click(90,168); // Appearance
        click(713,194); // Light theme
        click(713,383); // Semibold
        click(90,238); // Metrics & labels
        if(!IsWindowVisible(GetDlgItem(window.GetHwnd(),1001)))++failures;
        const bool was_enabled=settings.items[tmp::DisplayItem::UploadSpeed].enabled;
        click(480,288); // Per-metric unit checkbox must not toggle metric visibility.
        SetWindowTextW(GetDlgItem(window.GetHwnd(),1001),L"Upload");
        click(690,181); // Size & spacing
        if(IsWindowVisible(GetDlgItem(window.GetHwnd(),1001)))++failures;
        click(760,232); // Per-metric font size from inherited 0 to minimum 6.
        click(530,181); // Content
        if(!IsWindowVisible(GetDlgItem(window.GetHwnd(),1001)))++failures;
        click(90,377); // Alerts
        click(750,171); // Usage alerts
        click(90,481); // Startup & updates
        click(750,370); // Check for updates
        click(735,565);
        if(settings.theme!=L"light"||!settings.font_bold||!settings.notifications_enabled||
           settings.check_updates||settings.items[tmp::DisplayItem::UploadSpeed].custom_label!=L"Upload"||
           settings.items[tmp::DisplayItem::UploadSpeed].enabled!=was_enabled||
           settings.items[tmp::DisplayItem::UploadSpeed].font_size!=6){
            std::cerr<<"Moved settings were lost when switching categories\n";++failures;
        }
        if(IsWindowVisible(GetDlgItem(window.GetHwnd(),1001)))++failures;
        click(90,273); // Value colours
        click(750,166); // Automatic colours off
        click(90,168); // Appearance cross-link
        click(690,470); // Value colours link
        if(window.CurrentPage()!=6)++failures;
        click(735,565);
        if(settings.thermal_colors){std::cerr<<"Colour behaviour did not save\n";++failures;}
        click(650,212); // Restore automatic colours
        click(735,565);
        if(!settings.thermal_colors||!settings.custom_value_colors.empty())++failures;
        int sensor_opens=0,startup_repairs=0;
        window.on_sensor_status=[&](){++sensor_opens;};
        window.on_repair_startup=[&](){++startup_repairs;};
        click(90,412);click(330,380); // Sensor status remains reachable.
        click(90,481);click(330,265); // Startup repair remains reachable.
        if(sensor_opens!=1||startup_repairs!=1)++failures;
        const int prior_cpu_limit=settings.cpu_alert_threshold;
        click(90,377);click(470,312); // Increase CPU alert threshold.
        click(90,273);click(735,565); // Save from Value colours, which shares the threshold.
        if(settings.cpu_alert_threshold!=(std::min)(100,prior_cpu_limit+5))++failures;
        click(90,203); // Revisit position tab and verify native sliders return.
        if(!IsWindowVisible(GetDlgItem(window.GetHwnd(),1100)))++failures;
        click(350,128); // Arrangement
        if(IsWindowVisible(GetDlgItem(window.GetHwnd(),1100)))++failures;

        // Data usage ("which app/service, how much, when") interactive checks
        tmp::UsageTimeRange last_range=tmp::UsageTimeRange::Today;
        tmp::UsageNetworkScope last_scope=tmp::UsageNetworkScope::ActiveOnly;
        int force_calls=0;
        window.detailed_usage_snapshot=[&](tmp::UsageTimeRange r,tmp::UsageNetworkScope s,bool force){
            last_range=r;last_scope=s;if(force)++force_calls;
            tmp::NetworkUsageReport rep{};
            rep.ready=true;
            rep.range=r;rep.scope=s;
            rep.profile_label=(s==tmp::UsageNetworkScope::AllNetworks)?L"All networks (2)":L"Wi-Fi";
            rep.today_summary={2500000000ULL,1600000000ULL};
            rep.month_summary={27500000000ULL,6400000000ULL};
            rep.total_download=1200000000ULL;rep.total_upload=300000000ULL;
            rep.updated_at=L"01:30:00";
            rep.timeline={
                {L"00:00",L"00:00 - 01:00",500000000ULL,100000000ULL},
                {L"01:00",L"01:00 - 02:00",700000000ULL,200000000ULL}
            };
            rep.active_services={L"Delivery Optimization (DoSvc)",L"Connected Devices (CDPSvc)"};
            for(int k=0;k<7;++k){
                tmp::AppUsageEntry e;
                e.id=std::format(L"item_{}.exe",k);
                e.display_name=(k%2==0)?std::format(L"App Number {}",k):L"Windows Background Services";
                e.detail=e.id;
                e.category=(k%2==0)?tmp::AppUsageCategory::App:tmp::AppUsageCategory::Service;
                e.download=100000000ULL*(7-k);
                e.upload=20000000ULL*(7-k);
                e.peak_when=L"01:00 - 02:00";
                e.last_when=L"01:00 - 02:00";
                e.timeline=rep.timeline;
                rep.items.push_back(std::move(e));
            }
            return rep;
        };
        click(90,342); // Data usage tab
        if(window.CurrentPage()!=4||last_range!=tmp::UsageTimeRange::Today||last_scope!=tmp::UsageNetworkScope::ActiveOnly){
            std::cerr<<"Failed opening Data usage tab\n";++failures;
        }
        click(238,184); // Last 1h
        if(last_range!=tmp::UsageTimeRange::LastHour){std::cerr<<"Failed Last 1h switch\n";++failures;}
        click(369,184); // 7 Days
        if(last_range!=tmp::UsageTimeRange::Last7Days){std::cerr<<"Failed 7 Days switch\n";++failures;}
        click(446,184); // This Month
        if(last_range!=tmp::UsageTimeRange::ThisMonth){std::cerr<<"Failed This Month switch\n";++failures;}
        click(743,184); // Scope toggle -> All nets
        if(last_scope!=tmp::UsageNetworkScope::AllNetworks){std::cerr<<"Failed All nets toggle\n";++failures;}
        click(576,184); // Apps filter
        click(642,184); // Services filter
        click(523,184); // All filter
        click(370,255); // Select first app row
        click(610,315); // Pin first timeline bucket
        click(754,220); // Show all reset
        click(497,461); // Next >
        SendMessageW(window.GetHwnd(),WM_MOUSEWHEEL,MAKEWPARAM(0,-WHEEL_DELTA),0);
        UpdateWindow(window.GetHwnd());
        click(249,461); // < Prev
        click(551,506); // Refresh
        if(force_calls<1){std::cerr<<"Refresh button did not trigger forced snapshot\n";++failures;}

        SendMessageW(window.GetHwnd(),WM_ACTIVATE,WA_INACTIVE,0);
        if(!window.IsVisible())++failures;
        SendMessageW(window.GetHwnd(),WM_KEYDOWN,VK_ESCAPE,0);
        if(window.IsVisible())++failures;
        window.Show(settings);
        SendMessageW(window.GetHwnd(),WM_CLOSE,0,0);
        if(window.IsVisible())++failures;
    }
    CoUninitialize();
    if(failures)std::cerr<<failures<<" settings interaction failures\n";
    return failures?1:0;
}
