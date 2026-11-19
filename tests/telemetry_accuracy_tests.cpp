#include "pch.h"
#include "telemetry/measurement_validation.h"
#include "telemetry/usage_accounting.h"
#include "telemetry/gpu_usage.h"
#include "telemetry/cpu_temperature.h"
#include "telemetry/network_monitor.h"
#include "telemetry/memory_monitor.h"
#include "utils/string_utils.h"
#include <iostream>

int main(){
    int failures=0;
    auto check=[&](bool ok,const char* name){if(!ok){++failures;std::cerr<<"FAIL: "<<name<<"\n";}};
    using namespace tmp;
    check(!PreferWindowsUsage(true,{0,0},{1000,200}),"delayed Windows zero never hides measured traffic");
    check(PreferWindowsUsage(true,{1500,300},{1000,200}),"complete native usage can supersede tracked partial history");
    check(!PreferWindowsUsage(false,{1500,300},{1000,200}),"previous date is never selected");
    check(!PreferWindowsUsage(true,{2000,0},{1000,200}),"one direction cannot hide the other direction's measured bytes");
    check(CpuBusyPercent(600,800,200)==40,"CPU idle is already included in kernel");
    check(CpuBusyPercent(100,100,0)==0&&CpuBusyPercent(0,100,0)==100,"real CPU idle/busy boundaries");
    check(CpuBusyPercent(0,0,0)<0&&CpuBusyPercent(101,100,0)<0,"invalid CPU counters are unavailable");
    auto ram=ValidateMemory(16ULL<<30,6ULL<<30);
    check(ram.used==(10ULL<<30)&&ram.percent==62.5,"RAM bytes and percentage agree");
    check(ValidateMemory(0,0).percent<0&&ValidateMemory(10,11).percent<0,"invalid RAM never underflows");
    const double unknown=std::numeric_limits<double>::quiet_NaN();
    check(AggregateBatteryWatts({{L"pack1",true,-12.5},{L"pack2",true,-7.5}})==-20,"multi-pack discharge is summed");
    check(AggregateBatteryWatts({{L"PACK1",true,12},{L"pack1",true,12}})==12,"battery aliases counted once");
    check(AggregateBatteryWatts({{L"pack1",true,12},{L"ups",false,100}})==12,"UPS is not a laptop battery");
    check(std::isnan(AggregateBatteryWatts({{L"a",true,10},{L"b",true,unknown}})),"missing pack rate is not a partial total");
    check(std::isnan(AggregateBatteryWatts({}))&&AggregateBatteryWatts({{L"a",true,0}})==0,"unknown differs from confirmed zero watts");
    check(!CanUseUnidentifiedGpuSensor(2)&&CanUseUnidentifiedGpuSensor(1),"hybrid GPU bridge readings need identity");
    check(CanUseNvidiaSensor(0x10de,1,1)&&!CanUseNvidiaSensor(0x8086,1,1)&&!CanUseNvidiaSensor(0x10de,2,2),"GPU usage and temperature must match");
    check(GpuAdapterKey(L"luid_0x00000000_0x0000C6F4_phys_0_eng_9")==L"luid_0x00000000_0x0000c6f4","GPU adapter IDs normalize PDH hexadecimal casing");
    check(SelectCpuTemperature({{L"Distance to TjMax",20.0f},{L"CPU Package",52.0f},{L"Core #1",70.0f}}).value==52,"CPU package is selected instead of distance or a random core");
    check(SelectCpuTemperature({{L"CPU Package",NAN},{L"ACPI",55.0f}}).value<0,"unidentified/invalid temperatures are not guessed");
    UsageWindowAccumulator day;
    const auto key=UsageIdentity(L"adapter-1",L"WiFi");
    day.Add(key,{100,20});day.Add(UsageIdentity(L"ADAPTER-1",L"WiFi"),{100,20});
    day.Add(UsageIdentity(L"adapter-2",L"WiFi"),{30,4});
    check(day.total.download==130&&day.total.upload==24,"profile aliases deduplicated while distinct adapters remain distinct");
    check(UsageIdentity(L"adapter-1",L"WiFi")!=UsageIdentity(L"adapter-1",L"wifi"),"case-distinct networks are not merged");
    auto attributed=ValidateAttribution({{L"APP",L"App",{60,10}},{L"APP",L"App",{60,10}}},{100,20});
    check(attributed.size()==1&&attributed[0].bytes.download==60,"identical attribution rows counted once");
    bool rejected=false;try{ValidateAttribution({{L"a",L"",{80,0}},{L"b",L"",{30,0}}},{100,0});}catch(...){rejected=true;}
    check(rejected,"attribution above interface total is rejected, never scaled or double-added");
    rejected=false;try{ValidateAttribution({{L"a",L"",{30,0}},{L"a",L"",{40,0}}},{100,0});}catch(...){rejected=true;}
    check(rejected,"conflicting duplicate attribution is rejected");
    // A real rate uses elapsed time, never the configured polling interval.
    MIB_IF_TABLE2 table{};table.NumEntries=1;auto& nic=table.Table[0];
    nic.InterfaceIndex=31;nic.InterfaceLuid.Value=310;nic.OperStatus=IfOperStatusUp;nic.Type=IF_TYPE_ETHERNET_CSMACD;
    wcscpy_s(nic.Description,L"Accuracy test adapter");
    NetworkMonitor network(L"Accuracy test adapter");auto time=std::chrono::steady_clock::now();
    network.PollWithTable(&table,time);
    check(network.GetDownloadBytesPerSec()<0,"a baseline is unavailable, not a fake zero rate");
    nic.InOctets+=1000;nic.OutOctets+=250;
    network.PollWithTable(&table,time+std::chrono::milliseconds(250));
    check(network.GetDownloadBytesPerSec()==4000&&network.GetUploadBytesPerSec()==1000,"sub-second network rates use actual elapsed time");
    network.PollWithTable(&table,time+std::chrono::milliseconds(250));
    check(network.GetTotalDownloadedBytes()==1000,"repeated snapshot never adds bytes twice");
    // Same interface index reused for another adapter must establish a baseline.
    nic.InterfaceLuid.Value=311;nic.InOctets=1000000;
    network.PollWithTable(&table,time+std::chrono::seconds(1));
    check(network.GetTotalDownloadedBytes()==1000,"reused adapter index cannot manufacture a traffic spike");
    check(network.GetDownloadBytesPerSec()<0,"adapter replacement requires a new measured interval");
    nic.InOctets=0;nic.OutOctets=0;
    network.PollWithTable(&table,time+std::chrono::seconds(2));
    check(network.GetDownloadBytesPerSec()<0&&network.GetUploadBytesPerSec()<0,"reset counters are not confirmed zero traffic");
    check(FormatPercent(NAN)==L"--"&&FormatTemperature(INFINITY)==L"--"&&FormatWattage(unknown)==L"--","non-finite readings never become numeric display values");
    MemoryMonitor live;live.Poll();MEMORYSTATUSEX os{sizeof(os)};
    if(GlobalMemoryStatusEx(&os))check(live.GetTotalBytes()==os.ullTotalPhys&&live.GetUsagePercent()>=0&&live.GetUsagePercent()<=100,"live RAM matches native Windows total");
    std::cout<<(failures?"Accuracy checks failed\n":"All telemetry accuracy checks passed\n");
    return failures?1:0;
}
