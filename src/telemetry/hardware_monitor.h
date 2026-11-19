// TrafficMonitorPlus - Hardware Sensor Monitor (CPU/GPU Temp & Load)
#pragma once
#include "pch.h"
#include <pdh.h>
#include <pdhmsg.h>
#include <thread>
#include <condition_variable>
#include "../lib/OpenHardwareMonitor/OpenHardwareMonitorApi.h"

namespace tmp {
struct HardwareReadout {
    double cpu_temperature=-1, gpu_temperature=-1, gpu_usage=-1;
    std::wstring gpu_prefix=L"GPU:";
};

class HardwareMonitor {
public:
    HardwareMonitor();
    ~HardwareMonitor();

    void Init();
    void Poll() {} // Readout is sampled by the worker, never by the UI thread.
    bool WaitForFirstSample(std::chrono::milliseconds timeout);
    void SetInterval(int milliseconds) { m_interval_ms = (std::max)(500, milliseconds); }
    void SetTemperatureRequests(bool cpu, bool gpu) { m_cpu_requested=cpu; m_gpu_requested=gpu; }
    void RequestAdapterRescan(){m_rescan_adapters=true;}

    double GetCpuTemp() const { std::lock_guard lock(m_snapshot_mutex); return m_published_cpu; }
    double GetGpuUsage() const { std::lock_guard lock(m_snapshot_mutex); return m_published_usage; }
    double GetGpuTemp() const { std::lock_guard lock(m_snapshot_mutex); return m_published_gpu; }
    std::wstring GetGpuPrefix() const { std::lock_guard lock(m_snapshot_mutex); return m_published_prefix; }
    HardwareReadout GetReadout() const {
        std::lock_guard lock(m_snapshot_mutex);
        if(!m_ready||std::chrono::steady_clock::now()-m_published_at>std::chrono::milliseconds((std::max)(3000,m_interval_ms.load()*3)))return {};
        return {m_published_cpu,m_published_gpu,m_published_usage,m_published_prefix};
    }

    bool IsAvailable() const { std::lock_guard lock(m_snapshot_mutex); return m_published_available; }
    std::wstring GetStatus() const;

private:
    void InitializeSensors();
    void ReadSensors();
    void Publish();
    void CloseSensors();
    std::wstring BuildStatus() const;
    std::thread m_worker;
    std::atomic<bool> m_stop{false};
    std::atomic<bool> m_rescan_adapters{false};
    std::atomic<int> m_interval_ms{700};
    std::atomic<bool> m_cpu_requested{true},m_gpu_requested{true};
    bool m_managed_cpu=false,m_managed_gpu=false;
    ULONGLONG m_last_managed_attempt=0;
    std::mutex m_wait_mutex;
    std::condition_variable m_wake, m_ready_cv;
    mutable std::mutex m_snapshot_mutex;
    bool m_ready=false, m_published_available=false;
    std::chrono::steady_clock::time_point m_published_at{};
    double m_published_cpu=-1,m_published_gpu=-1,m_published_usage=-1;
    std::wstring m_published_prefix=L"GPU:", m_published_status=L"Sensors are initializing in the background...";
    bool TryInitOpenHardware();
    bool TryInitNvApi();
    void PollNvApi();

    bool InitPdhGpu();
    void PollPdhGpu();
    void DetectGpuType();
    std::wstring GetActiveGpuLabel(const std::wstring& engine);

    bool m_available = false;
    bool m_using_ohm = false;
    bool m_using_nvapi = false;

    double m_cpu_temp = -1.0;
    double m_gpu_usage = -1.0;
    double m_gpu_temp = -1.0;
    std::wstring m_gpu_prefix = L"dGPU: ";

    HQUERY m_gpu_query = nullptr;
    HCOUNTER m_gpu_counter = nullptr;
    ULONGLONG m_gpu_retry = 0;
    std::wstring m_sensor_error = L"Unavailable";
    std::wstring m_cpu_temperature_source=L"Unavailable";
    std::wstring m_gpu_error;
    std::wstring m_gpu_source = L"Unavailable";
    std::wstring m_gpu_temperature_source=L"Unavailable";
    std::wstring m_active_gpu_engine;
    std::map<std::wstring, std::wstring> m_gpu_labels;
    std::map<std::wstring,unsigned> m_gpu_vendors;
    size_t m_hardware_gpu_count=0,m_nvidia_gpu_count=0;

    std::shared_ptr<OpenHardwareMonitorApi::IOpenHardwareMonitor> m_ohm;

    // NVAPI dynamic function pointers
    HMODULE m_hNvApi = nullptr;
    typedef int* (*NvAPI_QueryInterface_t)(unsigned int offset);
    NvAPI_QueryInterface_t m_nvapi_QueryInterface = nullptr;

    typedef int (*NvAPI_Initialize_t)();
    NvAPI_Initialize_t m_NvAPI_Initialize = nullptr;

    typedef int (*NvAPI_EnumPhysicalGPUs_t)(int** handles, int* count);
    NvAPI_EnumPhysicalGPUs_t m_NvAPI_EnumPhysicalGPUs = nullptr;

    typedef int (*NvAPI_GPU_GetDynamicPstatesInfo_t)(int* handle, void* info);
    NvAPI_GPU_GetDynamicPstatesInfo_t m_NvAPI_GPU_GetDynamicPstatesInfo = nullptr;

    typedef int (*NvAPI_GPU_GetThermalSettings_t)(int* handle, int sensorIndex, void* pThermalSettings);
    NvAPI_GPU_GetThermalSettings_t m_NvAPI_GPU_GetThermalSettings = nullptr;

    int* m_nvGpuHandles[64]{};
    int  m_nvGpuCount = 0;
};

} // namespace tmp
