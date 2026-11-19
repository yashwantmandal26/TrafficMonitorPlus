#pragma once
#include "telemetry/live_network_types.h"
#include <evntrace.h>
#include <evntcons.h>
#include <thread>

namespace tmp {
struct NetworkPacket { DWORD pid; uint32_t size; bool upload; };
// Only the documented v2 TCP/UDP transfer events; never process I/O counters.
std::optional<NetworkPacket> DecodeNetworkPacket(const EVENT_RECORD& event, bool include_loopback = false);

class LiveNetworkMonitor {
public:
    explicit LiveNetworkMonitor(bool include_loopback = false);
    ~LiveNetworkMonitor();
    LiveNetworkMonitor(const LiveNetworkMonitor&) = delete;
    LiveNetworkMonitor& operator=(const LiveNetworkMonitor&) = delete;
    bool Start();
    void Stop();
    // Called by the app timer even while the panel is hidden. No work in paint.
    void Poll(bool force = false);
    const LiveNetworkSnapshot& Snapshot() const { return m_snapshot; }
    bool LoadHistory(const fs::path& path);
    bool SaveHistory(const fs::path& path) const;
private:
    friend struct LiveNetworkMonitorTestAccess;
    static void WINAPI OnEvent(EVENT_RECORD* event);
    static ULONG WINAPI OnBuffer(EVENT_TRACE_LOGFILEW* logfile);
    void Record(const NetworkPacket& packet, uint32_t day = 0);
    void Drain(std::chrono::steady_clock::time_point now, const std::wstring& day);
    static std::pair<std::wstring,std::wstring> ResolveProcess(DWORD pid, uint64_t* created = nullptr);
    struct Bytes { uint64_t download = 0, upload = 0; };
    struct Properties { EVENT_TRACE_PROPERTIES trace{}; wchar_t name[128]{}; };
    Properties m_properties{};
    std::wstring m_session_name, m_day;
    TRACEHANDLE m_session = 0, m_consumer = INVALID_PROCESSTRACE_HANDLE;
    std::thread m_thread;
    std::atomic<bool> m_stopping{false};
    std::atomic<ULONG> m_consumer_error{ERROR_SUCCESS};
    std::atomic<uint64_t> m_lost_events{0};
    std::mutex m_mutex;
    std::map<std::pair<uint32_t,DWORD>,Bytes> m_pending;
    std::map<std::wstring,LiveNetworkEntry> m_totals;
    LiveNetworkSnapshot m_snapshot;
    std::chrono::steady_clock::time_point m_last_poll{};
    bool m_include_loopback = false, m_history_writable = true;
};
}
