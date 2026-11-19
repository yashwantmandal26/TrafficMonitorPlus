#include "telemetry/live_network_monitor.h"
#include <iostream>
#include <cstring>

namespace tmp {
struct LiveNetworkMonitorTestAccess {
    static void record(LiveNetworkMonitor& m, DWORD pid, uint32_t bytes, bool up, uint32_t day=0){m.Record({pid,bytes,up},day);}
    static void drain(LiveNetworkMonitor& m,double seconds,const std::wstring& day){auto now=std::chrono::steady_clock::now();m.m_last_poll=now-std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(seconds));m.Drain(now,day);}
};
}
namespace {
constexpr GUID tcp{0x9a280ac0,0xc8e0,0x11d1,{0x84,0xe2,0x00,0xc0,0x4f,0xb9,0x98,0xa2}};
constexpr GUID udp{0xbf3a50c5,0xa9c9,0x4988,{0xa0,0x05,0x2d,0xf0,0xb7,0xc8,0x0f,0x80}};
std::wstring day(){SYSTEMTIME t{};GetLocalTime(&t);return std::format(L"{:04}-{:02}-{:02}",t.wYear,t.wMonth,t.wDay);}
void dump(const tmp::LiveNetworkSnapshot& s){
    std::wcout<<L"State="<<int(s.state)<<L" "<<s.message<<L" rows="<<s.entries.size()<<L" lost="<<s.lost_events<<L"\n";
    auto rows=s.entries;std::sort(rows.begin(),rows.end(),[](auto& a,auto& b){return a.today_download+a.today_upload>b.today_download+b.today_upload;});
    for(size_t i=0;i<(std::min)(rows.size(),size_t{5});++i){const auto& e=rows[i];std::wcout<<e.name<<L" down="<<e.download_bps<<L" B/s up="<<e.upload_bps<<L" B/s captured-down="<<e.today_download<<L" captured-up="<<e.today_upload<<L"\n";}
}
int integration(bool probe){
    WSADATA w{};WSAStartup(MAKEWORD(2,2),&w);
    tmp::LiveNetworkMonitor monitor(!probe);
    if(!monitor.Start()){dump(monitor.Snapshot());WSACleanup();return 1;}
    if(probe){for(int i=0;i<10;++i){Sleep(1000);monitor.Poll();dump(monitor.Snapshot());}monitor.Stop();WSACleanup();return monitor.Snapshot().entries.empty()?2:0;}
    SOCKET receiver=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP),sender=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=0;
    DWORD timeout=2000;setsockopt(receiver,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&timeout),sizeof(timeout));
    if(bind(receiver,reinterpret_cast<sockaddr*>(&address),sizeof(address))!=0)return 2;
    int length=sizeof(address);getsockname(receiver,reinterpret_cast<sockaddr*>(&address),&length);
    std::vector<char> bytes(1200,'x'),received(1500);
    uint64_t sent=0,got=0;bool live_rate=false;
    // A loopback-only fixture. No data leaves this computer.
    for(int round=0;round<5;++round){
        for(int i=0;i<100;++i){int n=sendto(sender,bytes.data(),int(bytes.size()),0,reinterpret_cast<sockaddr*>(&address),sizeof(address));if(n>0)sent+=n;n=recv(receiver,received.data(),int(received.size()),0);if(n>0)got+=n;}
        Sleep(1100);monitor.Poll();for(const auto& e:monitor.Snapshot().entries)if(e.name==L"live_network_tests.exe"&&(e.upload_bps>0||e.download_bps>0))live_rate=true;
    }
    Sleep(1500);monitor.Poll();dump(monitor.Snapshot());
    uint64_t up=0,down=0;for(const auto& e:monitor.Snapshot().entries)if(e.name==L"live_network_tests.exe"){up+=e.today_upload;down+=e.today_download;}
    monitor.Stop();closesocket(sender);closesocket(receiver);WSACleanup();
    std::cout<<"Owned UDP fixture: sent="<<sent<<" received="<<got<<" ETW upload="<<up<<" download="<<down<<" observed-live-rate="<<live_rate<<"\n";
    return sent==600000&&got==600000&&up>=sent&&down>=got&&live_rate?0:3;
}
}
int main(int argc,char** argv){
    CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if(argc>1){const int result=integration(std::string(argv[1])=="--probe");CoUninitialize();return result;}
    int failures=0;auto check=[&](bool value,const char* label){if(!value){std::cerr<<label<<"\n";++failures;}};
    std::array<BYTE,80> payload{};DWORD pid=1234,size=8192;std::memcpy(payload.data(),&pid,4);std::memcpy(payload.data()+4,&size,4);payload[8]=10;payload[12]=10;
    EVENT_RECORD e{};e.EventHeader.EventDescriptor.Version=2;e.EventHeader.ProcessId=9999;e.UserData=payload.data();e.UserDataLength=payload.size();
    for(auto guid:{tcp,udp})for(BYTE op:{10,11,26,27}){
        e.EventHeader.ProviderId=guid;e.EventHeader.EventDescriptor.Opcode=op;
        const auto p=tmp::DecodeNetworkPacket(e);check(p&&p->pid==1234&&p->size==8192&&p->upload==(op==10||op==26),"TCP/UDP IPv4/IPv6 payload PID and direction");
    }
    for(BYTE op:{12,13,14,18,28,30}){e.EventHeader.EventDescriptor.Opcode=op;check(!tmp::DecodeNetworkPacket(e),"retransmit/connect/copy must not double-count");}
    e.EventHeader.EventDescriptor.Opcode=10;e.UserDataLength=7;check(!tmp::DecodeNetworkPacket(e),"truncated record rejected");e.UserDataLength=payload.size();
    e.EventHeader.EventDescriptor.Version=0;check(!tmp::DecodeNetworkPacket(e),"unknown schema rejected");e.EventHeader.EventDescriptor.Version=2;
    payload[8]=127;check(!tmp::DecodeNetworkPacket(e),"IPv4 loopback excluded");check(tmp::DecodeNetworkPacket(e,true).has_value(),"owned integration may include loopback");
    tmp::LiveNetworkMonitor m;const auto current=day();
    tmp::LiveNetworkMonitorTestAccess::record(m,GetCurrentProcessId(),1200,true);
    tmp::LiveNetworkMonitorTestAccess::record(m,GetCurrentProcessId(),2400,false);
    tmp::LiveNetworkMonitorTestAccess::drain(m,2,current);
    check(m.Snapshot().entries.size()==1,"process attributed");
    if(!m.Snapshot().entries.empty()){const auto& r=m.Snapshot().entries[0];check(std::abs(r.upload_bps-600)<.01&&std::abs(r.download_bps-1200)<.01,"elapsed-time rates");check(r.today_download==2400&&r.today_upload==1200,"captured totals");}
    tmp::LiveNetworkMonitorTestAccess::drain(m,2,current);
    check(m.Snapshot().entries[0].upload_bps==0&&m.Snapshot().entries[0].download_bps==0&&m.Snapshot().entries[0].today_download==2400,"idle rates reset but totals persist");
    const auto history=fs::temp_directory_path()/(L"TrafficMonitor-live-test-"+std::to_wstring(GetCurrentProcessId())+L".json");
    check(m.SaveHistory(history),"save history");tmp::LiveNetworkMonitor restored;check(restored.LoadHistory(history),"restore history");tmp::LiveNetworkMonitorTestAccess::drain(restored,2,current);check(restored.Snapshot().entries.size()==1&&restored.Snapshot().entries[0].today_download==2400,"restart retains day totals");
    tmp::LiveNetworkMonitorTestAccess::record(m,GetCurrentProcessId(),100,true,20990101);
    tmp::LiveNetworkMonitorTestAccess::record(m,GetCurrentProcessId(),700,true,20990102);
    tmp::LiveNetworkMonitorTestAccess::drain(m,1,L"2099-01-02");check(m.Snapshot().entries.size()==1&&m.Snapshot().entries[0].today_upload==700,"midnight excludes old buffered packets and resets day");
    std::error_code ignored;fs::remove(history,ignored);CoUninitialize();
    if(!failures)std::cout<<"Live TCP/UDP decoding, rates, idle, midnight and persistence passed\n";
    return failures?1:0;
}
