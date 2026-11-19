// TrafficMonitorPlus - Memory Monitor Implementation
#include "pch.h"
#include "telemetry/memory_monitor.h"
#include "telemetry/measurement_validation.h"

namespace tmp {

// ---------------------------------------------------------------------------
// Poll - query system memory status
// ---------------------------------------------------------------------------
void MemoryMonitor::Poll() {
    MEMORYSTATUSEX ms = {};
    ms.dwLength = sizeof(ms);

    if (!GlobalMemoryStatusEx(&ms)||ms.ullTotalPhys==0){
        m_usage_percent=-1;m_total_bytes=m_available_bytes=m_used_bytes=0;
        return;
    }

    const auto reading=ValidateMemory(ms.ullTotalPhys,ms.ullAvailPhys);
    m_total_bytes=reading.total;m_available_bytes=reading.available;
    m_used_bytes=reading.used;m_usage_percent=reading.percent;
}

} // namespace tmp
