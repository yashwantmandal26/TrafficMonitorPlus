// TrafficMonitorPlus - CPU Monitor
// Tracks CPU usage via GetSystemTimes and frequency via NtPowerInformation.
#pragma once
#include "../pch.h"

namespace tmp {

class CpuMonitor {
public:
    CpuMonitor()  = default;
    ~CpuMonitor(){if(m_group_query)PdhCloseQuery(m_group_query);}

    CpuMonitor(const CpuMonitor&) = delete;
    CpuMonitor& operator=(const CpuMonitor&) = delete;

    void Init();
    void Poll();

    double GetUsagePercent()  const { return m_usage_percent; }
    double GetFrequencyGHz()  const { return m_frequency_ghz; }

private:
    void UpdateFrequency();
    void PollGroupedUsage();
    bool m_multiple_groups=false;
    HQUERY m_group_query=nullptr;
    HCOUNTER m_group_counter=nullptr;
    ULONGLONG m_group_retry=0;

    ULONGLONG m_prev_idle   = 0;
    ULONGLONG m_prev_kernel = 0;
    ULONGLONG m_prev_user   = 0;

    double m_usage_percent  = -1.0;
    double m_frequency_ghz  = -1.0;

    bool m_initialized = false;
};

} // namespace tmp
