// TrafficMonitorPlus - CPU Monitor Implementation
#include "pch.h"
#include "telemetry/cpu_monitor.h"
#include "telemetry/measurement_validation.h"
#include <pdhmsg.h>

// PROCESSOR_POWER_INFORMATION is defined in ntstatus.h / powerbase.h
// but we need the struct definition ourselves.
namespace {

struct PROCESSOR_POWER_INFORMATION {
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
};

// Convert FILETIME to uint64 (100-ns intervals)
inline ULONGLONG FileTimeToU64(const FILETIME& ft) {
    ULARGE_INTEGER li;
    li.LowPart  = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    return li.QuadPart;
}

} // anonymous namespace

namespace tmp {

// ---------------------------------------------------------------------------
// Init - capture baseline system times
// ---------------------------------------------------------------------------
void CpuMonitor::Init() {
    m_multiple_groups=GetActiveProcessorGroupCount()>1;
    if(m_multiple_groups){PollGroupedUsage();UpdateFrequency();return;}
    FILETIME idle, kernel, user;
    if (GetSystemTimes(&idle, &kernel, &user)) {
        m_prev_idle   = FileTimeToU64(idle);
        m_prev_kernel = FileTimeToU64(kernel);
        m_prev_user   = FileTimeToU64(user);
    }else{m_usage_percent=-1;m_initialized=false;UpdateFrequency();return;}

    m_usage_percent = -1.0;
    UpdateFrequency();
    m_initialized = true;
}

// ---------------------------------------------------------------------------
// Poll - calculate CPU usage from time deltas
// ---------------------------------------------------------------------------
void CpuMonitor::Poll() {
    if(m_multiple_groups){PollGroupedUsage();UpdateFrequency();return;}
    FILETIME idle, kernel, user;
    if (!GetSystemTimes(&idle, &kernel, &user)){
        m_usage_percent=-1;m_initialized=false;UpdateFrequency();
        return;
    }

    ULONGLONG cur_idle   = FileTimeToU64(idle);
    ULONGLONG cur_kernel = FileTimeToU64(kernel);
    ULONGLONG cur_user   = FileTimeToU64(user);
    if(cur_idle<m_prev_idle||cur_kernel<m_prev_kernel||cur_user<m_prev_user){m_initialized=false;m_usage_percent=-1;}

    if (!m_initialized) {
        m_prev_idle   = cur_idle;
        m_prev_kernel = cur_kernel;
        m_prev_user   = cur_user;
        m_initialized = true;
        UpdateFrequency();
        return;
    }

    ULONGLONG idle_delta   = cur_idle   - m_prev_idle;
    ULONGLONG kernel_delta = cur_kernel - m_prev_kernel;
    ULONGLONG user_delta   = cur_user   - m_prev_user;

    // kernel time includes idle time
    m_usage_percent=CpuBusyPercent(idle_delta,kernel_delta,user_delta);

    m_prev_idle   = cur_idle;
    m_prev_kernel = cur_kernel;
    m_prev_user   = cur_user;

    UpdateFrequency();
}

// ---------------------------------------------------------------------------
// UpdateFrequency - query current processor MHz via NtPowerInformation
// ---------------------------------------------------------------------------
void CpuMonitor::UpdateFrequency() {
    // ProcessorInformation does not identify processor groups. Do not label a
    // primary-group average as a whole-system frequency on >64-CPU machines.
    if(m_multiple_groups){m_frequency_ghz=-1;return;}
    SYSTEM_INFO si = {};
    GetSystemInfo(&si);
    DWORD num_procs = si.dwNumberOfProcessors;
    if (num_procs == 0)
        num_procs = 1;

    std::vector<PROCESSOR_POWER_INFORMATION> info(num_procs);
    ULONG buffer_size = static_cast<ULONG>(sizeof(PROCESSOR_POWER_INFORMATION) * num_procs);

    NTSTATUS status = CallNtPowerInformation(
        ProcessorInformation,
        nullptr, 0,
        info.data(), buffer_size
    );

    if (status == 0 /* STATUS_SUCCESS */) {
        // Average current MHz across all cores
        double total_mhz = 0.0;
        for (DWORD i = 0; i < num_procs; ++i) {
            if(info[i].CurrentMhz==0){m_frequency_ghz=-1;return;}
            total_mhz += static_cast<double>(info[i].CurrentMhz);
        }
        m_frequency_ghz = (total_mhz / static_cast<double>(num_procs)) / 1000.0;
    } else {
        m_frequency_ghz = -1.0;
    }
}

void CpuMonitor::PollGroupedUsage(){
    m_usage_percent=-1;
    if(!m_group_query){
        const auto now=GetTickCount64();
        if(m_group_retry&&now-m_group_retry<10000)return;
        m_group_retry=now;
        auto status=PdhOpenQueryW(nullptr,0,&m_group_query);
        if(status==ERROR_SUCCESS)status=PdhAddEnglishCounterW(m_group_query,
            L"\\Processor Information(_Total)\\% Processor Time",0,&m_group_counter);
        if(status!=ERROR_SUCCESS){if(m_group_query)PdhCloseQuery(m_group_query);m_group_query=nullptr;return;}
        PdhCollectQueryData(m_group_query);return; // Establish a real counter baseline.
    }
    PDH_FMT_COUNTERVALUE value{};
    if(PdhCollectQueryData(m_group_query)!=ERROR_SUCCESS||
       PdhGetFormattedCounterValue(m_group_counter,PDH_FMT_DOUBLE,nullptr,&value)!=ERROR_SUCCESS){
        PdhCloseQuery(m_group_query);m_group_query=nullptr;return;
    }
    if((value.CStatus==PDH_CSTATUS_VALID_DATA||value.CStatus==PDH_CSTATUS_NEW_DATA)&&
       std::isfinite(value.doubleValue)&&value.doubleValue>=0&&value.doubleValue<=100)
        m_usage_percent=value.doubleValue;
}

} // namespace tmp
