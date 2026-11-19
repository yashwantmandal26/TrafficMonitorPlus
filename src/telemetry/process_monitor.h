// TrafficMonitorPlus - Process Monitor (Top CPU & Bandwidth Hogs)
#pragma once
#include "pch.h"
#include "types.h"
#include <future>

namespace tmp {

/// Strict weak ordering comparator for processes:
/// Exact lexicographic ordering by CPU% descending, memory descending, PID ascending.
/// Satisfies irreflexivity, asymmetry, transitivity, and deterministic tie-breaking.
bool CompareProcessCpuUsage(const ProcessInfo& a, const ProcessInfo& b) noexcept;

class ProcessMonitor {
public:
    explicit ProcessMonitor(std::function<std::vector<ProcessInfo>()> sampler = {});
    ~ProcessMonitor();

    void Poll();

    const std::vector<ProcessInfo>& GetTopCpuProcesses() const { return m_top_cpu; }
    const std::vector<ProcessInfo>& GetProcesses() const { return m_processes; }

private:
    std::vector<ProcessInfo> SampleProcesses();
    std::function<std::vector<ProcessInfo>()> m_sampler;
    std::future<std::vector<ProcessInfo>> m_worker;
    struct ProcessCpuHistory {
        FILETIME created{};
        FILETIME last_kernel{};
        FILETIME last_user{};
        std::chrono::steady_clock::time_point last_time;
    };

    std::map<DWORD, ProcessCpuHistory> m_history;
    std::vector<ProcessInfo> m_top_cpu;
    std::vector<ProcessInfo> m_processes;
    DWORD m_num_processors = 1;
    std::chrono::steady_clock::time_point m_last_poll;
};

} // namespace tmp
