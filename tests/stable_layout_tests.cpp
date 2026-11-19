#include "pch.h"
#include "config/settings.h"
#include "rendering/d2d_renderer.h"
#include "rendering/text_renderer.h"
#include "rendering/theme.h"
#include <iostream>
int main() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    HWND hwnd = CreateWindowExW(0, L"STATIC", L"Layout regression test", WS_POPUP, 0,0,1200,120,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    int failures=0;
    {
        tmp::D2DRenderer renderer;
        if(!hwnd || !renderer.Init(hwnd)) return 1;
        renderer.SetFont(L"Segoe UI",13,true);
        tmp::Theme theme; theme.Init(true);
        tmp::TextRenderer text(&renderer,&theme);
        auto settings=tmp::Settings::GetDefaults();
        tmp::TelemetryData low{},high{};
        low.gpu_prefix=L"iGPU:";low.cpu_usage_percent=1;low.cpu_temp_celsius=35;
        high.gpu_prefix=L"dGPU:";high.cpu_usage_percent=100;high.cpu_temp_celsius=95;
        high.download_bytes_per_sec=5000000000.0;high.upload_bytes_per_sec=999999;
        auto items=settings.GetEnabledItemsSorted();
        auto first=text.PrepareItems(low,items,true);
        auto second=text.PrepareItems(high,items,true);
        if(first.size()!=second.size())return 1;
        bool changing_color=false;
        auto same=[](D2D1_COLOR_F a,D2D1_COLOR_F b){return std::abs(a.r-b.r)<0.001f&&std::abs(a.g-b.g)<0.001f&&std::abs(a.b-b.b)<0.001f;};
        for(size_t i=0;i<first.size();++i) {
            if(first[i].width!=second[i].width || first[i].label_width!=second[i].label_width || first[i].value_width!=second[i].value_width)++failures;
            if(!same(second[i].label_color, theme.GetLabelColor()))++failures;
            if(first[i].value_color.r!=second[i].value_color.r || first[i].value_color.g!=second[i].value_color.g)changing_color=true;
        }
        if(!changing_color)++failures;
        for(double speed:{0.0,99.95*1024,999.99*1024,99.95*1024*1024,999.9*1024*1024,10.0*1024*1024*1024}) {
            auto sample=low;sample.download_bytes_per_sec=speed;
            auto prepared=text.PrepareItems(sample,{{tmp::DisplayItem::DownloadSpeed,{true,0,L""}}},true);
            if(renderer.MeasureText(prepared.front().value).width>prepared.front().value_width)++failures;
        }
        // Explicit configured boundary checks, independent of live measurements.
        settings.cpu_alert_threshold=80;settings.gpu_color_threshold=80;
        settings.temp_alert_threshold=60;settings.download_color_threshold_mb=10;
        text.SetThresholds(settings);
        auto colourFor=[&](tmp::DisplayItem metric,double value) {
            auto sample=low;
            sample.cpu_usage_percent=value;sample.gpu_usage_percent=value;
            sample.cpu_temp_celsius=value;sample.download_bytes_per_sec=value*1024*1024;
            auto prepared=text.PrepareItems(sample,{{metric,{true,0,L""}}},true);
            return prepared.front().value_color;
        };
        // Assert actual palette colours, rather than deriving expectations from the palette.
        const auto white=D2D1::ColorF(D2D1::ColorF::White);
        const auto yellow=tmp::Theme::ColorRefToD2D(RGB(255,216,58));
        const auto orange=tmp::Theme::ColorRefToD2D(RGB(255,140,0));
        const auto red=tmp::Theme::ColorRefToD2D(RGB(255,23,68));
        if(!same(theme.GetLabelColor(),white)||!same(theme.GetAccentColor(),white)){
            std::cerr<<"Default labels/arrows must be white\n";++failures;
        }
        settings.cpu_alert_threshold=settings.ram_alert_threshold=settings.gpu_color_threshold=80;
        settings.temp_alert_threshold=80;
        settings.download_color_threshold_mb=settings.upload_color_threshold_mb=80;
        settings.battery_color_threshold_w=settings.frequency_color_threshold_ghz=80;
        text.SetThresholds(settings);
        const std::map<tmp::DisplayItem,COLORREF> custom_values={{tmp::DisplayItem::CpuUsage,RGB(0,255,0)}};
        for(auto metric:{tmp::DisplayItem::CpuUsage,tmp::DisplayItem::GpuUsage,tmp::DisplayItem::RamUsage,
                         tmp::DisplayItem::CpuTemp,tmp::DisplayItem::GpuTemp,
                         tmp::DisplayItem::BatteryWattage,tmp::DisplayItem::CpuFrequency,
                         tmp::DisplayItem::DownloadSpeed,tmp::DisplayItem::UploadSpeed}) {
            auto color=[&](double value){
                auto sample=low;
                sample.cpu_usage_percent=sample.gpu_usage_percent=sample.ram_usage_percent=value;
                sample.cpu_temp_celsius=sample.gpu_temp_celsius=value;
                sample.battery_wattage=-value;sample.cpu_freq_ghz=value;
                sample.download_bytes_per_sec=sample.upload_bytes_per_sec=value*1024*1024;
                auto prepared=text.PrepareItems(sample,{{metric,{true,0,L""}}},true,nullptr,&custom_values);
                return prepared.front().value_color;
            };
            const bool network=metric==tmp::DisplayItem::DownloadSpeed||metric==tmp::DisplayItem::UploadSpeed;
            const double warm=network?150.0/1024:40.0;
            const double hot=network?(metric==tmp::DisplayItem::DownloadSpeed?16.0:20.0):60.0;
            for(auto [value,expected]:std::vector<std::pair<double,D2D1_COLOR_F>>{
                    {0,white},{warm,yellow},{hot,orange},{80,red},{120,red}}) {
                if(!same(color(value),expected)) {
                    std::cerr<<"Incorrect intensity colour for "<<tmp::GetItemKey(metric)<<" at "<<value<<"\n";
                    ++failures;
                }
            }
            // Each band must contain intermediate shades, and crossing its
            // boundary by a tiny amount must never cause a visible colour jump.
            double start=0;
            for(double end:{warm,hot,80.0}){
                const auto middle=color((start+end)/2);
                if(same(middle,color(start))||same(middle,color(end))){
                    std::cerr<<"Colour still snaps within a band\n";++failures;
                }
                const auto before=color(end-0.00001),after=color(end+0.00001);
                if(!same(before,after)){
                    std::cerr<<"Discontinuous colour at boundary\n";++failures;
                }
                start=end;
            }
        }
        settings.cpu_alert_threshold=80;settings.gpu_color_threshold=80;
        settings.temp_alert_threshold=60;settings.download_color_threshold_mb=10;
        text.SetThresholds(settings);
        if(same(colourFor(tmp::DisplayItem::CpuUsage,39),theme.GetThermalColor(0)) ||
           !same(colourFor(tmp::DisplayItem::CpuUsage,40),theme.GetThermalColor(.25f)) ||
           !same(colourFor(tmp::DisplayItem::CpuUsage,60),theme.GetThermalColor(.5f)) ||
           !same(colourFor(tmp::DisplayItem::CpuUsage,80),theme.GetThermalColor(1)) ||
           !same(colourFor(tmp::DisplayItem::CpuTemp,60),theme.GetThermalColor(1)) ||
           !same(colourFor(tmp::DisplayItem::DownloadSpeed,10),theme.GetThermalColor(1)) ||
           !same(colourFor(tmp::DisplayItem::GpuUsage,80),theme.GetThermalColor(1)))++failures;
        renderer.BeginDraw();
        const auto width1=text.RenderTwoRows(first,0,0,8,20);
        const auto width2=text.RenderTwoRows(second,0,0,8,20);
        renderer.EndDraw();
        if(width1!=width2)++failures;
        settings.items[tmp::DisplayItem::GpuUsage].font_size=18;
        settings.items[tmp::DisplayItem::GpuUsage].value_width=120;
        settings.items[tmp::DisplayItem::CpuUsage].show_unit=false;
        settings.items[tmp::DisplayItem::CpuTemp].show_unit=true;
        auto customized=text.PrepareItems(high,settings.GetEnabledItemsSorted(),true);
        for(const auto& item:customized){
            if(item.item==tmp::DisplayItem::GpuUsage && (item.font_size!=18 || item.value_width!=120))++failures;
            if(item.item==tmp::DisplayItem::CpuUsage && item.value.find(L'%')!=std::wstring::npos)++failures;
            if(item.item==tmp::DisplayItem::CpuTemp && item.value.find(L"\u00b0C")==std::wstring::npos)++failures;
        }
    }
    DestroyWindow(hwnd); CoUninitialize();
    if(failures)std::cerr<<failures<<" layout failures\n";
    return failures?1:0;
}
