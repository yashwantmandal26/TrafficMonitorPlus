// TrafficMonitorPlus - Process Monitor Implementation
#include "pch.h"
#include "telemetry/process_monitor.h"

namespace tmp {

static uint64_t FileTimeToUInt64(const FILETIME& ft) {
    ULARGE_INTEGER uli{};
    uli.LowPart  = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart;
}

bool CompareProcessCpuUsage(const ProcessInfo& a, const ProcessInfo& b) noexcept {
    if (a.cpu_percent != b.cpu_percent) {
        return a.cpu_percent > b.cpu_percent;
    }
    if (a.memory_bytes != b.memory_bytes) {
        return a.memory_bytes > b.memory_bytes;
    }
    return a.pid < b.pid;
}

ProcessMonitor::ProcessMonitor(std::function<std::vector<ProcessInfo>()> sampler)
    : m_sampler(std::move(sampler)) {
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    m_num_processors = (std::max)(1UL, GetActiveProcessorCount(ALL_PROCESSOR_GROUPS));
    m_last_poll = std::chrono::steady_clock::now() - std::chrono::milliseconds(1500);
}

ProcessMonitor::~ProcessMonitor() {
    // The worker owns access to CPU history until it finishes. Join before any
    // members it uses are destroyed; it never waits on the window thread.
    if (m_worker.valid()) m_worker.wait();
}

void ProcessMonitor::Poll() {
    // Called on the UI thread: publish only completed, immutable snapshots.
    if (m_worker.valid()) {
        if (m_worker.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
        try {
            m_processes = m_worker.get();
            m_top_cpu.assign(m_processes.begin(), m_processes.begin() + (std::min)(size_t{6}, m_processes.size()));
        } catch (...) { /* Keep the last successful snapshot. */ }
    }
    auto now = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_poll).count();
    if (elapsed_ms < 1500) {
        return; // Throttle process snapshot to every 1.5 seconds to minimize CPU usage
    }
    m_last_poll = now;
    try {
        m_worker = std::async(std::launch::async, [this]() {
            auto processes = m_sampler ? m_sampler() : SampleProcesses();
            std::sort(processes.begin(), processes.end(), CompareProcessCpuUsage);
            return processes;
        });
    } catch (...) { /* Retry on the next sampling interval. */ }
}

std::vector<ProcessInfo> ProcessMonitor::SampleProcesses() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Process snapshot failed");
    }

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    std::vector<ProcessInfo> current_list;
    std::set<DWORD> active_pids;

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (pe.th32ProcessID == 0 || pe.th32ProcessID == 4) {
                continue; // Skip Idle and System
            }

            active_pids.insert(pe.th32ProcessID);

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
            if (!hProcess) {
                continue;
            }

            FILETIME ftCreate{}, ftExit{}, ftKernel{}, ftUser{};
            if (GetProcessTimes(hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser)) {
                const auto sampled_at=std::chrono::steady_clock::now();
                auto it = m_history.find(pe.th32ProcessID);
                if (it != m_history.end() && CompareFileTime(&it->second.created, &ftCreate) == 0) {
                    uint64_t prev_time = FileTimeToUInt64(it->second.last_kernel) + FileTimeToUInt64(it->second.last_user);
                    uint64_t cur_time  = FileTimeToUInt64(ftKernel) + FileTimeToUInt64(ftUser);
                    const double dt_ms = std::chrono::duration<double,std::milli>(sampled_at - it->second.last_time).count();

                    if (dt_ms > 0 && cur_time >= prev_time) {
                        uint64_t delta_100ns = cur_time - prev_time;
                        // 1 ms = 10,000 * 100ns
                        double cpu_pct = (static_cast<double>(delta_100ns) / (static_cast<double>(dt_ms) * 10000.0 * static_cast<double>(m_num_processors))) * 100.0;
                        ProcessInfo pi{};
                        pi.pid = pe.th32ProcessID;
                        pi.name = pe.szExeFile;
                        pi.cpu_percent = (std::min)(100.0, (std::max)(0.0, cpu_pct));
                        PROCESS_MEMORY_COUNTERS_EX pmc{};
                        if (K32GetProcessMemoryInfo(hProcess, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
                            pi.memory_bytes = pmc.WorkingSetSize;
                        }
                        if (pi.cpu_percent > 0.1 || pi.memory_bytes > 15 * 1024 * 1024) {
                            current_list.push_back(std::move(pi));
                        }
                    }

                    it->second.last_kernel = ftKernel;
                    it->second.last_user   = ftUser;
                    it->second.last_time   = sampled_at;
                } else {
                    ProcessCpuHistory pch{};
                    pch.created = ftCreate;
                    pch.last_kernel = ftKernel;
                    pch.last_user   = ftUser;
                    pch.last_time   = sampled_at;
                    m_history[pe.th32ProcessID] = pch;

                    ProcessInfo pi{};
                    pi.pid = pe.th32ProcessID;
                    pi.name = pe.szExeFile;
                    pi.cpu_percent = 0.0;
                    PROCESS_MEMORY_COUNTERS_EX pmc{};
                    if (K32GetProcessMemoryInfo(hProcess, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
                        pi.memory_bytes = pmc.WorkingSetSize;
                    }
                    if (pi.memory_bytes > 20 * 1024 * 1024) {
                        current_list.push_back(std::move(pi));
                    }
                }
            }

            CloseHandle(hProcess);
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);

    // Clean up dead PIDs from history
    for (auto it = m_history.begin(); it != m_history.end(); ) {
        if (!active_pids.contains(it->first)) {
            it = m_history.erase(it);
        } else {
            ++it;
        }
    }

    return current_list;
}

} // namespace tmp
