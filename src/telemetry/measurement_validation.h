#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <vector>
#include <cwchar>
#include <format>
#include <string_view>

namespace tmp {
inline double CpuBusyPercent(uint64_t idle, uint64_t kernel, uint64_t user) {
    // Windows includes idle in kernel time. Reject inconsistent counters rather
    // than clamping an invalid measurement into a plausible percentage.
    const long double total=static_cast<long double>(kernel)+user;
    if(total<=0||idle>total)return -1;
    return static_cast<double>(100*(total-idle)/total);
}
struct MemoryReadout { uint64_t total=0,available=0,used=0;double percent=-1; };
inline MemoryReadout ValidateMemory(uint64_t total,uint64_t available){
    if(!total||available>total)return {};
    return {total,available,total-available,100.0*static_cast<double>(total-available)/total};
}
struct BatteryRateSample {
    std::wstring id;
    bool system_battery=true;
    double watts=std::numeric_limits<double>::quiet_NaN();
};
inline double AggregateBatteryWatts(const std::vector<BatteryRateSample>& samples){
    std::map<std::wstring,double> unique;
    for(const auto& sample:samples){
        if(!sample.system_battery)continue;
        if(!std::isfinite(sample.watts))return std::numeric_limits<double>::quiet_NaN();
        auto id=sample.id;
        for(auto& c:id)if(c>=L'A'&&c<=L'Z')c+=L'a'-L'A';
        const auto [it,added]=unique.emplace(id,sample.watts);
        if(!added&&it->second!=sample.watts)return std::numeric_limits<double>::quiet_NaN();
    }
    if(unique.empty())return std::numeric_limits<double>::quiet_NaN();
    double total=0;
    for(const auto& [id,watts]:unique)total+=watts;
    return std::isfinite(total)?total:std::numeric_limits<double>::quiet_NaN();
}
// GPU bridge APIs have no adapter ID. Only a single hardware adapter can make
// an unlabelled bridge reading unambiguous on hybrid/multi-GPU machines.
inline bool CanUseUnidentifiedGpuSensor(size_t hardware_adapters){return hardware_adapters==1;}
inline bool CanUseNvidiaSensor(unsigned active_vendor,size_t nvidia_adapters,int nvapi_adapters){
    return active_vendor==0x10DE&&nvidia_adapters==1&&nvapi_adapters==1;
}
inline std::wstring GpuAdapterKey(std::wstring_view engine){
    unsigned high=0,low=0;
    const std::wstring text(engine);
    if(std::swscanf(text.c_str(),L"luid_0x%x_0x%x",&high,&low)!=2)return {};
    return std::format(L"luid_0x{:08x}_0x{:08x}",high,low);
}
}
