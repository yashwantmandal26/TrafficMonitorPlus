// TrafficMonitorPlus - Network Monitor
// Monitors network adapter traffic using the IP Helper API.
#pragma once
#include "../pch.h"

namespace tmp {

class NetworkMonitor {
public:
    explicit NetworkMonitor(const std::wstring& adapter_filter = {});
    ~NetworkMonitor() = default;

    NetworkMonitor(const NetworkMonitor&) = delete;
    NetworkMonitor& operator=(const NetworkMonitor&) = delete;

    void Init();
    void Poll();

    double GetDownloadBytesPerSec() const { return m_download_bps; }
    double GetUploadBytesPerSec()   const { return m_upload_bps; }
    uint64_t GetTotalDownloadedBytes() const { return m_total_download_bytes; }
    uint64_t GetTotalUploadedBytes()   const { return m_total_upload_bytes; }
    std::wstring GetAdapterName()   const { return m_adapter_name; }
    std::wstring GetLocalIP()       const { return m_local_ip; }
    bool IsConnected()              const { return m_adapter_connected; }
    bool IsManualSelection()        const { return m_is_manual_selection; }

    std::vector<std::wstring> GetAvailableAdapters() const;
    void SetAdapterFilter(const std::wstring& name);
    void UpdateLocalIP();

    // Deterministic testing seam with simulated table and time
    void PollWithTable(PMIB_IF_TABLE2 table, std::chrono::steady_clock::time_point now);

private:
    void SelectAdapter(PMIB_IF_TABLE2 table);
    void ResetBaseline(const MIB_IF_ROW2& row, std::chrono::steady_clock::time_point now);
    void ComputeRates(const MIB_IF_ROW2& row, std::chrono::steady_clock::time_point now);
    void MarkDisconnected();
    bool MatchesFilter(const MIB_IF_ROW2& row, const std::wstring& filter) const;

    std::wstring m_adapter_filter;
    std::wstring m_adapter_name;
    std::wstring m_local_ip;
    bool m_is_manual_selection = false;
    bool m_adapter_connected = false;

    NET_IFINDEX  m_adapter_index = 0;
    NET_LUID     m_adapter_luid  = {};

    uint64_t m_prev_in_bytes  = 0;
    uint64_t m_prev_out_bytes = 0;
    std::chrono::steady_clock::time_point m_prev_time;

    double m_download_bps = -1.0;
    double m_upload_bps   = -1.0;
    uint64_t m_total_download_bytes = 0;
    uint64_t m_total_upload_bytes   = 0;

    bool m_initialized = false;
    NET_IFINDEX m_ip_adapter_index = 0;
    std::chrono::steady_clock::time_point m_ip_refresh{};
};

} // namespace tmp
