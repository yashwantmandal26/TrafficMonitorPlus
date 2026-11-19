// TrafficMonitorPlus - Memory Monitor
// Reports system memory usage via GlobalMemoryStatusEx.
#pragma once
#include "../pch.h"

namespace tmp {

class MemoryMonitor {
public:
    MemoryMonitor()  = default;
    ~MemoryMonitor() = default;

    MemoryMonitor(const MemoryMonitor&) = delete;
    MemoryMonitor& operator=(const MemoryMonitor&) = delete;

    void Poll();

    double   GetUsagePercent()   const { return m_usage_percent; }
    uint64_t GetUsedBytes()      const { return m_used_bytes; }
    uint64_t GetTotalBytes()     const { return m_total_bytes; }
    uint64_t GetAvailableBytes() const { return m_available_bytes; }

private:
    double   m_usage_percent   = -1.0;
    uint64_t m_used_bytes      = 0;
    uint64_t m_total_bytes     = 0;
    uint64_t m_available_bytes = 0;
};

} // namespace tmp
