#include "telemetry/live_network_monitor.h"
#include "utils/string_utils.h"
#include "json.hpp"
#include <cstring>

namespace tmp {
namespace {
constexpr GUID kTcp{0x9a280ac0,0xc8e0,0x11d1,{0x84,0xe2,0x00,0xc0,0x4f,0xb9,0x98,0xa2}};
constexpr GUID kUdp{0xbf3a50c5,0xa9c9,0x4988,{0xa0,0x05,0x2d,0xf0,0xb7,0xc8,0x0f,0x80}};
std::wstring Day() { SYSTEMTIME t{}; GetLocalTime(&t); return std::format(L"{:04}-{:02}-{:02}",t.wYear,t.wMonth,t.wDay); }
std::wstring Clock() { SYSTEMTIME t{}; GetLocalTime(&t); return std::format(L"{:02}:{:02}",t.wHour,t.wMinute); }
uint32_t DayNumber(const std::wstring& day) { return std::stoi(day.substr(0,4))*10000+std::stoi(day.substr(5,2))*100+std::stoi(day.substr(8,2)); }
bool IsLoopback6(const BYTE* p) { for(int i=0;i<15;++i)if(p[i])return false;return p[15]==1; }
void EnableProfilePrivilege() {
    HANDLE token=nullptr;
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&token))return;
    TOKEN_PRIVILEGES p{};p.PrivilegeCount=1;p.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
    if(LookupPrivilegeValueW(nullptr,SE_SYSTEM_PROFILE_NAME,&p.Privileges[0].Luid)) AdjustTokenPrivileges(token,FALSE,&p,0,nullptr,nullptr);
    CloseHandle(token);
}
}

std::optional<NetworkPacket> DecodeNetworkPacket(const EVENT_RECORD& e, bool include_loopback) {
    if((!IsEqualGUID(e.EventHeader.ProviderId,kTcp)&&!IsEqualGUID(e.EventHeader.ProviderId,kUdp)) || e.EventHeader.EventDescriptor.Version!=2)return {};
    const auto op=e.EventHeader.EventDescriptor.Opcode;
    if(op!=10&&op!=11&&op!=26&&op!=27)return {}; // Exclude retransmit/connect/copy; do not double count.
    const bool ipv6=op==26||op==27;
    if(!e.UserData||e.UserDataLength<(ipv6?44:20))return {};
    const auto data=static_cast<const BYTE*>(e.UserData);
    if(!include_loopback && (ipv6?(IsLoopback6(data+8)||IsLoopback6(data+24)):(data[8]==127||data[12]==127)))return {};
    NetworkPacket p{};std::memcpy(&p.pid,data,4);std::memcpy(&p.size,data+4,4);
    p.upload=op==10||op==26;
    if(!p.pid||!p.size)return {};
    return p;
}

LiveNetworkMonitor::LiveNetworkMonitor(bool include_loopback):m_include_loopback(include_loopback) {}
LiveNetworkMonitor::~LiveNetworkMonitor() { Stop(); }
bool LiveNetworkMonitor::Start() {
    if(m_session)return true;
    EnableProfilePrivilege();
    m_properties={};m_consumer_error=ERROR_SUCCESS;m_stopping=false;m_lost_events=0;
    m_session_name=std::format(L"TrafficMonitorPlus.Network.{}.{}",GetCurrentProcessId(),GetTickCount64());
    auto& p=m_properties.trace;
    p.Wnode.BufferSize=sizeof(m_properties);p.Wnode.Flags=WNODE_FLAG_TRACED_GUID;p.Wnode.ClientContext=1;
    CoCreateGuid(&p.Wnode.Guid); // Private system logger; never hijack the NT Kernel Logger.
    p.LogFileMode=EVENT_TRACE_REAL_TIME_MODE|EVENT_TRACE_SYSTEM_LOGGER_MODE;
    p.EnableFlags=EVENT_TRACE_FLAG_NETWORK_TCPIP|EVENT_TRACE_FLAG_NO_SYSCONFIG;
    p.BufferSize=64;p.MinimumBuffers=16;p.MaximumBuffers=64;p.FlushTimer=1;
    p.LoggerNameOffset=offsetof(Properties,name);
    const auto error=StartTraceW(&m_session,m_session_name.c_str(),&p);
    auto fail=[&](ULONG code){m_snapshot.state=LiveNetworkState::Unavailable;m_snapshot.message=code==ERROR_ACCESS_DENIED||code==ERROR_PRIVILEGE_NOT_HELD?L"Live tracking needs administrator access":std::format(L"Live tracking unavailable (Windows error {})",code);return false;};
    if(error!=ERROR_SUCCESS){m_session=0;return fail(error);}
    EVENT_TRACE_LOGFILEW log{};log.LoggerName=m_session_name.data();log.ProcessTraceMode=PROCESS_TRACE_MODE_REAL_TIME|PROCESS_TRACE_MODE_EVENT_RECORD;
    log.EventRecordCallback=&OnEvent;log.BufferCallback=&OnBuffer;log.Context=this;
    m_consumer=OpenTraceW(&log);
    if(m_consumer==INVALID_PROCESSTRACE_HANDLE){const auto code=GetLastError();Stop();return fail(code);}
    m_last_poll=std::chrono::steady_clock::now();
    if(m_day!=Day()){m_day=Day();m_totals.clear();m_snapshot.coverage_start=Clock();}
    if(m_snapshot.coverage_start.empty())m_snapshot.coverage_start=Clock();
    m_snapshot.state=LiveNetworkState::Live;m_snapshot.message=L"Live app traffic \u00B7 all adapters";
    try { m_thread=std::thread([this](){const auto code=ProcessTrace(&m_consumer,1,nullptr,nullptr);if(!m_stopping)m_consumer_error=code==ERROR_SUCCESS?ERROR_INVALID_HANDLE:code;}); }
    catch(...) {Stop();return fail(ERROR_NOT_ENOUGH_MEMORY);}
    return true;
}
void LiveNetworkMonitor::Stop() {
    m_stopping=true;
    if(m_session){ControlTraceW(m_session,m_session_name.c_str(),&m_properties.trace,EVENT_TRACE_CONTROL_STOP);m_session=0;}
    if(m_consumer!=INVALID_PROCESSTRACE_HANDLE){CloseTrace(m_consumer);}
    if(m_thread.joinable())m_thread.join();
    m_consumer=INVALID_PROCESSTRACE_HANDLE;
    if(m_snapshot.state==LiveNetworkState::Live)m_snapshot.state=LiveNetworkState::Stopped;
}
void WINAPI LiveNetworkMonitor::OnEvent(EVENT_RECORD* e) {
    if(!e||!e->UserContext)return;
    auto self=static_cast<LiveNetworkMonitor*>(e->UserContext);
    // No exception may cross an ETW C callback boundary.
    try {if(auto packet=DecodeNetworkPacket(*e,self->m_include_loopback)){
        // ProcessTrace converts QPC timestamps to FILETIME (RAW_TIMESTAMP is not enabled).
        // Use event time, not buffer-delivery time, across local midnight.
        const FILETIME utc{e->EventHeader.TimeStamp.LowPart,DWORD(e->EventHeader.TimeStamp.HighPart)};
        FILETIME local{};SYSTEMTIME time{};
        if(FileTimeToLocalFileTime(&utc,&local)&&FileTimeToSystemTime(&local,&time))
            self->Record(*packet,time.wYear*10000+time.wMonth*100+time.wDay);
    }}catch(...){++self->m_lost_events;}
}
ULONG WINAPI LiveNetworkMonitor::OnBuffer(EVENT_TRACE_LOGFILEW* log) {
    if(log&&log->Context){auto self=static_cast<LiveNetworkMonitor*>(log->Context);if(log->EventsLost)self->m_lost_events=(std::max)(self->m_lost_events.load(),uint64_t(log->EventsLost));}
    return TRUE;
}
void LiveNetworkMonitor::Record(const NetworkPacket& p, uint32_t day) {
    if(!day)day=DayNumber(Day());
    std::lock_guard lock(m_mutex);
    auto& bytes=m_pending[{day,p.pid}];if(p.upload)bytes.upload+=p.size;else bytes.download+=p.size;
}
std::pair<std::wstring,std::wstring> LiveNetworkMonitor::ResolveProcess(DWORD pid, uint64_t* created) {
    if(pid==4)return {L"System",L"System"};
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
    wchar_t path[32768]{};DWORD length=_countof(path);
    const bool ok=process&&QueryFullProcessImageNameW(process,0,path,&length);
    FILETIME birth{},exit{},kernel{},user{};
    if(created&&process&&GetProcessTimes(process,&birth,&exit,&kernel,&user))*created=(uint64_t(birth.dwHighDateTime)<<32)|birth.dwLowDateTime;
    if(process)CloseHandle(process);
    if(ok){std::wstring key=path;std::transform(key.begin(),key.end(),key.begin(),towlower);return {key,fs::path(path).filename().wstring()};}
    // Unresolved traffic stays visible without inventing an app/service name.
    return {L"pid:"+std::to_wstring(pid),std::format(L"PID {} (unresolved)",pid)};
}
void LiveNetworkMonitor::Drain(std::chrono::steady_clock::time_point now,const std::wstring& day) {
    const double seconds=std::chrono::duration<double>(now-m_last_poll).count();if(seconds<=0)return;
    std::map<std::pair<uint32_t,DWORD>,Bytes> pending;{std::lock_guard lock(m_mutex);pending.swap(m_pending);}
    if(m_day!=day){m_totals.clear();m_day=day;m_snapshot.coverage_start=Clock();}
    for(auto& [id,row]:m_totals){row.download_bps=row.upload_bps=0;row.processes.clear();}
    for(const auto& [key,bytes]:pending){
        if(key.first!=DayNumber(day))continue;
        const auto pid=key.second;
        uint64_t created=0;
        const auto [id,name]=ResolveProcess(pid,&created);auto& row=m_totals[id];row.id=id;row.name=name;
        if(created)row.processes.push_back({pid,created});
        row.download_bps+=bytes.download/seconds;row.upload_bps+=bytes.upload/seconds;
        row.today_download+=bytes.download;row.today_upload+=bytes.upload;
    }
    m_snapshot.entries.clear();for(const auto& [id,row]:m_totals)m_snapshot.entries.push_back(row);
    m_snapshot.sampled_at=now;m_snapshot.lost_events=m_lost_events;m_last_poll=now;
}
void LiveNetworkMonitor::Poll(bool force) {
    const auto now=std::chrono::steady_clock::now();
    if(m_last_poll.time_since_epoch().count()==0)m_last_poll=now-std::chrono::seconds(1);
    if(!force&&now-m_last_poll<std::chrono::milliseconds(750))return;
    Drain(now,Day());
    if(m_consumer_error!=ERROR_SUCCESS){m_snapshot.state=LiveNetworkState::Unavailable;m_snapshot.message=std::format(L"Live capture stopped (Windows error {})",m_consumer_error.load());}
}
bool LiveNetworkMonitor::LoadHistory(const fs::path& path) {
    using json=nlohmann::json;
    try {
        if(!fs::exists(path))return true;
        std::ifstream file(path);const auto j=json::parse(file);
        if(j.at("version")!=1||!j.at("entries").is_array())throw std::runtime_error("schema");
        if(Utf8ToWide(j.at("date").get<std::string>())!=Day())return true;
        std::map<std::wstring,LiveNetworkEntry> loaded;
        for(const auto& e:j.at("entries")){
            if(!e.at("download").is_number_unsigned()||!e.at("upload").is_number_unsigned())throw std::runtime_error("entry");
            LiveNetworkEntry row{};row.id=Utf8ToWide(e.at("id").get<std::string>());row.name=Utf8ToWide(e.at("name").get<std::string>());
            row.today_download=e.at("download").get<uint64_t>();row.today_upload=e.at("upload").get<uint64_t>();loaded[row.id]=row;
        }
        m_snapshot.coverage_start=Utf8ToWide(j.at("coverage_start").get<std::string>());m_day=Day();m_totals=std::move(loaded);
        return true;
    } catch(...) {m_history_writable=false;return false;}
}
bool LiveNetworkMonitor::SaveHistory(const fs::path& path) const {
    using json=nlohmann::json;if(!m_history_writable||m_day.empty())return false;
    try {
        json entries=json::array();for(const auto& [id,row]:m_totals)entries.push_back({{"id",WideToUtf8(id)},{"name",WideToUtf8(row.name)},{"download",row.today_download},{"upload",row.today_upload}});
        fs::create_directories(path.parent_path());auto temp=path;temp+=L".tmp";
        std::ofstream file(temp,std::ios::binary|std::ios::trunc);file<<json{{"version",1},{"date",WideToUtf8(m_day)},{"coverage_start",WideToUtf8(m_snapshot.coverage_start)},{"entries",entries}}.dump(2);file.flush();const bool ok=file.good();file.close();
        return ok&&MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    }catch(...){return false;}
}
}
