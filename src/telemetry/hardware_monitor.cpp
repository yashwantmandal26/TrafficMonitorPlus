// TrafficMonitorPlus - Hardware Sensor Monitor Implementation
#include "pch.h"
#include "telemetry/hardware_monitor.h"
#include "telemetry/gpu_usage.h"
#include "telemetry/cpu_temperature.h"
#include "telemetry/measurement_validation.h"
#include <dxcore.h>
#include <dxgi1_2.h>

namespace tmp {

// NVAPI Thermal Settings structure
#pragma pack(push, 8)
struct NV_GPU_THERMAL_SETTINGS_V2 {
    unsigned int version;
    unsigned int count;
    struct {
        int controller;
        int defaultMinTemp;
        int defaultMaxTemp;
        int currentTemp;
        int target;
    } sensor[3];
};
struct NV_GPU_DYNAMIC_PSTATES_INFO {
    unsigned version,flags;
    struct {unsigned present:1;unsigned reserved:31;unsigned percentage;} utilization[8];
};
static_assert(sizeof(NV_GPU_THERMAL_SETTINGS_V2)==68);
static_assert(sizeof(NV_GPU_DYNAMIC_PSTATES_INFO)==72);
#pragma pack(pop)

HardwareMonitor::HardwareMonitor() = default;

HardwareMonitor::~HardwareMonitor() {
    m_stop = true;
    m_wake.notify_all();
    if (m_worker.joinable()) m_worker.join();
}

void HardwareMonitor::CloseSensors() {
    if (m_gpu_query) PdhCloseQuery(m_gpu_query);
    m_ohm.reset();
    if (m_hNvApi) {
        FreeLibrary(m_hNvApi);
        m_hNvApi = nullptr;
    }
}

void HardwareMonitor::Init() {
    if (m_worker.joinable()) return;
    m_worker = std::thread([this]() {
        const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        try {
            InitializeSensors();
            while (!m_stop) {
                const auto sample_started=std::chrono::steady_clock::now();
                ReadSensors(); Publish();
                std::unique_lock lock(m_wait_mutex);
                m_wake.wait_until(lock, sample_started+std::chrono::milliseconds(m_interval_ms.load()), [this]() { return m_stop.load(); });
            }
        } catch (...) {
            m_cpu_temp=m_gpu_temp=m_gpu_usage=-1;
            m_sensor_error=L"Sensor worker failed. Other monitoring remains available.";
            Publish();
        }
        CloseSensors();
        if (SUCCEEDED(com)) CoUninitialize();
    });
}

bool HardwareMonitor::WaitForFirstSample(std::chrono::milliseconds timeout) {
    std::unique_lock lock(m_snapshot_mutex);
    return m_ready_cv.wait_for(lock, timeout, [this]() { return m_ready; });
}

void HardwareMonitor::Publish() {
    {
        std::lock_guard lock(m_snapshot_mutex);
        m_published_cpu=m_cpu_temp; m_published_gpu=m_gpu_temp; m_published_usage=m_gpu_usage;
        m_published_available=m_available; m_published_prefix=m_gpu_prefix;
        m_published_status=BuildStatus(); m_ready=true;
        m_published_at=std::chrono::steady_clock::now();
    }
    m_ready_cv.notify_all();
}

void HardwareMonitor::InitializeSensors() {
    DetectGpuType();
    if (InitPdhGpu()) m_available = true;

    if (TryInitNvApi()) {
        m_available = true;
    }
    if ((m_cpu_requested || (m_gpu_requested && !m_using_nvapi)) && TryInitOpenHardware()) {
        m_available = true;
    }
}

void HardwareMonitor::DetectGpuType() {
    m_gpu_prefix=L"GPU:";m_hardware_gpu_count=m_nvidia_gpu_count=0;
    m_gpu_vendors.clear();m_gpu_labels.clear();
    HMODULE dxgi=LoadLibraryExW(L"dxgi.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!dxgi)return;
    using CreateFactory=HRESULT(WINAPI*)(REFIID,void**);
    auto create=reinterpret_cast<CreateFactory>(GetProcAddress(dxgi,"CreateDXGIFactory1"));
    {
        ComPtr<IDXGIFactory1> factory;
        if(create&&SUCCEEDED(create(IID_PPV_ARGS(&factory)))){
            for(UINT i=0;;++i){
                ComPtr<IDXGIAdapter1> adapter;
                if(factory->EnumAdapters1(i,&adapter)!=S_OK)break;
                DXGI_ADAPTER_DESC1 desc{};
                if(FAILED(adapter->GetDesc1(&desc))||(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE))continue;
                ++m_hardware_gpu_count;
                if(desc.VendorId==0x10DE)++m_nvidia_gpu_count;
                const auto key=std::format(L"luid_0x{:08x}_0x{:08x}",static_cast<unsigned>(desc.AdapterLuid.HighPart),desc.AdapterLuid.LowPart);
                m_gpu_vendors[key]=desc.VendorId;
            }
        }
    }
    FreeLibrary(dxgi);
    if(m_hardware_gpu_count==1)m_gpu_prefix=GetActiveGpuLabel(m_gpu_vendors.begin()->first);
}

bool HardwareMonitor::InitPdhGpu() {
    if (m_gpu_query) PdhCloseQuery(m_gpu_query);
    m_gpu_query = nullptr;
    m_gpu_counter = nullptr;
    auto status = PdhOpenQueryW(nullptr, 0, &m_gpu_query);
    if (status == ERROR_SUCCESS)
        status = PdhAddEnglishCounterW(m_gpu_query,
            L"\\GPU Engine(*)\\Utilization Percentage", 0, &m_gpu_counter);
    if (status != ERROR_SUCCESS) {
        if (m_gpu_query) PdhCloseQuery(m_gpu_query);
        m_gpu_query = nullptr;
        m_gpu_error = L"GPU performance counters unavailable: " + std::to_wstring(status);
        return false;
    }
    PdhCollectQueryData(m_gpu_query);
    return true;
}

void HardwareMonitor::PollPdhGpu() {
    if (!m_gpu_query) {
        const auto now = GetTickCount64();
        if (now - m_gpu_retry < 10000) return;
        m_gpu_retry = now;
        if (!InitPdhGpu()) return;
    }
    if (PdhCollectQueryData(m_gpu_query) != ERROR_SUCCESS) {
        PdhCloseQuery(m_gpu_query);
        m_gpu_query = nullptr;
        return;
    }
    DWORD bytes = 0, count = 0;
    auto status = PdhGetFormattedCounterArrayW(m_gpu_counter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100,
        &bytes, &count, nullptr);
    if (status != PDH_MORE_DATA || bytes == 0) return;
    std::vector<BYTE> buffer(bytes);
    auto items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
    status = PdhGetFormattedCounterArrayW(m_gpu_counter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100,
        &bytes, &count, items);
    if (status != ERROR_SUCCESS) return;
    std::vector<std::pair<std::wstring, double>> samples;
    for (DWORD i = 0; i < count; ++i) {
        if (items[i].szName && (items[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA ||
            items[i].FmtValue.CStatus == PDH_CSTATUS_NEW_DATA))
            samples.emplace_back(items[i].szName, items[i].FmtValue.doubleValue);
    }
    std::wstring selected_engine;
    const auto usage = AggregateGpuEngines(samples, &selected_engine, m_active_gpu_engine);
    if (usage >= 0) {
        m_gpu_usage = usage;
        m_gpu_source = L"Windows GPU Engine counters (busiest physical engine)";
        m_active_gpu_engine = selected_engine;
        m_gpu_prefix = GetActiveGpuLabel(selected_engine);
        m_gpu_error.clear();
    }
}

std::wstring HardwareMonitor::GetActiveGpuLabel(const std::wstring& engine) {
    const auto key = GpuAdapterKey(engine);
    if(key.empty())return L"GPU:";
    if (auto found = m_gpu_labels.find(key); found != m_gpu_labels.end()) return found->second;
    unsigned int high = 0, low = 0;
    std::wstring label = L"GPU:";
    if (swscanf_s(key.c_str(), L"luid_0x%x_0x%x", &high, &low) != 2) return label;
    // DXCore reports integrated/discrete for this exact LUID, including Intel
    // discrete and AMD integrated devices. Optional on older Windows 10 builds.
    HMODULE module = LoadLibraryExW(L"dxcore.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (module) {
        using CreateFactory = HRESULT(WINAPI*)(REFIID, void**);
        auto create = reinterpret_cast<CreateFactory>(GetProcAddress(module, "DXCoreCreateAdapterFactory"));
        {
            ComPtr<IDXCoreAdapterFactory> factory;
            ComPtr<IDXCoreAdapter> adapter;
            LUID luid{low, static_cast<LONG>(high)};
            bool integrated = false, hardware = false;
            if (create && SUCCEEDED(create(IID_PPV_ARGS(&factory))) &&
                SUCCEEDED(factory->GetAdapterByLuid(luid, IID_PPV_ARGS(&adapter))) &&
                SUCCEEDED(adapter->GetProperty(DXCoreAdapterProperty::IsHardware, &hardware)) && hardware &&
                SUCCEEDED(adapter->GetProperty(DXCoreAdapterProperty::IsIntegrated, &integrated)))
                label = integrated ? L"iGPU:" : L"dGPU:";
        }
        FreeLibrary(module);
    }
    m_gpu_labels[key] = label;
    return label;
}

std::wstring HardwareMonitor::GetStatus() const {
    std::lock_guard lock(m_snapshot_mutex);
    return m_published_status;
}

std::wstring HardwareMonitor::BuildStatus() const {
    return L"LibreHardwareMonitor: " + (m_using_ohm ? std::wstring(L"loaded") : L"unavailable") +
        L"\r\nProvider message: " + m_sensor_error +
        L"\r\nCPU temperature: " + std::to_wstring(m_cpu_temp) +
        L" C\r\nCPU source: " + m_cpu_temperature_source + L"\r\nGPU temperature: " + std::to_wstring(m_gpu_temp) +
        L" C\r\nGPU load: " + std::to_wstring(m_gpu_usage) + L" %\r\nGPU source: " + m_gpu_source +
        L"\r\nGPU temperature source: "+m_gpu_temperature_source+
        L"\r\nGPU label: " + m_gpu_prefix + L"\r\nActive engine: " + m_active_gpu_engine +
        L"\r\nHardware GPU adapters: "+std::to_wstring(m_hardware_gpu_count)+L" (NVIDIA: "+std::to_wstring(m_nvidia_gpu_count)+L")"+
        L"\r\n" + m_gpu_error +
        L"\r\nNegative values mean unavailable. CPU package sensors can require administrator rights "
        L"and a compatible driver. ACPI thermal zones are not reported as CPU temperature. "
        L"GPU temperatures without an unambiguous adapter match are unavailable.\r\n";
}

bool HardwareMonitor::TryInitOpenHardware() {
    m_last_managed_attempt=GetTickCount64();
    // Only attempt if OpenHardwareMonitorApi.dll loads cleanly
    HMODULE hDll = LoadLibraryW(L"OpenHardwareMonitorApi.dll");
    if (!hDll) {
        m_sensor_error = L"Bridge load failed: Windows error " + std::to_wstring(GetLastError()) +
            L". Check the Microsoft Visual C++ x64 runtime and DLL dependencies.";
        return false;
    }

    FreeLibrary(hDll);
    try {
        m_ohm = OpenHardwareMonitorApi::CreateInstance();
        if (m_ohm) {
            m_managed_cpu=m_cpu_requested;
            m_managed_gpu=m_gpu_requested && !m_using_nvapi;
            m_ohm->SetCpuEnable(m_managed_cpu);
            m_ohm->SetGpuEnable(m_managed_gpu);
            m_ohm->SetHddEnable(false);
            m_ohm->SetMainboardEnable(false);
            m_using_ohm = true;
            m_available = true;
            return true;
        }
        m_sensor_error = OpenHardwareMonitorApi::GetErrorMessage();
    } catch (...) {
        m_sensor_error = L"Sensor bridge initialization failed.";
        m_ohm.reset();
        m_using_ohm = false;
    }

    return false;
}

bool HardwareMonitor::TryInitNvApi() {
    m_hNvApi = LoadLibraryW(L"nvapi64.dll");
    if (!m_hNvApi) return false;

    m_nvapi_QueryInterface = reinterpret_cast<NvAPI_QueryInterface_t>(
        GetProcAddress(m_hNvApi, "nvapi_QueryInterface")
    );
    if (!m_nvapi_QueryInterface) {
        FreeLibrary(m_hNvApi);
        m_hNvApi = nullptr;
        return false;
    }

    m_NvAPI_Initialize = reinterpret_cast<NvAPI_Initialize_t>(
        m_nvapi_QueryInterface(0x0150E828)
    );
    m_NvAPI_EnumPhysicalGPUs = reinterpret_cast<NvAPI_EnumPhysicalGPUs_t>(
        m_nvapi_QueryInterface(0xE5AC921F)
    );
    m_NvAPI_GPU_GetDynamicPstatesInfo = reinterpret_cast<NvAPI_GPU_GetDynamicPstatesInfo_t>(
        m_nvapi_QueryInterface(0x60DED2ED)
    );
    m_NvAPI_GPU_GetThermalSettings = reinterpret_cast<NvAPI_GPU_GetThermalSettings_t>(
        m_nvapi_QueryInterface(0xE3640A56)
    );

    if (!m_NvAPI_Initialize || !m_NvAPI_EnumPhysicalGPUs) {
        FreeLibrary(m_hNvApi);
        m_hNvApi = nullptr;
        return false;
    }

    if (m_NvAPI_Initialize() != 0) {
        FreeLibrary(m_hNvApi);
        m_hNvApi = nullptr;
        return false;
    }

    if (m_NvAPI_EnumPhysicalGPUs(m_nvGpuHandles, &m_nvGpuCount) != 0 || m_nvGpuCount <= 0) {
        FreeLibrary(m_hNvApi);
        m_hNvApi = nullptr;
        return false;
    }

    m_using_nvapi = true;
    return true;
}

void HardwareMonitor::ReadSensors() {
    if(m_rescan_adapters.exchange(false))DetectGpuType();
    m_cpu_temp = m_gpu_temp = m_gpu_usage = -1.0;
    m_cpu_temperature_source=L"Unavailable";
    m_gpu_temperature_source=L"Unavailable";

    const bool need_cpu=m_cpu_requested;
    const bool need_gpu=m_gpu_requested && !m_using_nvapi;
    // Retry a bridge that loaded before its driver became ready, rather than
    // leaving a missing CPU reading unavailable for the entire session.
    if(need_cpu&&m_using_ohm&&GetCpuTemp()<0&&GetTickCount64()-m_last_managed_attempt>=30000){
        m_ohm.reset();m_using_ohm=false;
        TryInitOpenHardware();
    }
    if (!m_using_ohm && (need_cpu||need_gpu) &&
        (m_last_managed_attempt==0 || GetTickCount64()-m_last_managed_attempt>=60000)) TryInitOpenHardware();

    if (m_using_ohm && m_ohm && (need_cpu || need_gpu)) {
      try {
        if (need_cpu!=m_managed_cpu) {m_ohm->SetCpuEnable(need_cpu);m_managed_cpu=need_cpu;}
        if (need_gpu!=m_managed_gpu) {m_ohm->SetGpuEnable(need_gpu);m_managed_gpu=need_gpu;}
        m_ohm->GetHardwareInfo();

        m_sensor_error = OpenHardwareMonitorApi::GetErrorMessage();
        if(need_cpu){
            const auto cpu=SelectCpuTemperature(m_ohm->AllCpuTemperature());
            m_cpu_temp=cpu.value;m_cpu_temperature_source=cpu.source;
        }
        if(need_gpu){
            float gpu_t = m_ohm->GpuTemperature();
            if (std::isfinite(gpu_t) && gpu_t > 0.0f && gpu_t <= 150.0f){m_gpu_temp = static_cast<double>(gpu_t);m_gpu_temperature_source=L"LibreHardwareMonitor (single adapter)";}
            float gpu_u = m_ohm->GpuUsage();
            if (std::isfinite(gpu_u) && gpu_u >= 0.0f && gpu_u <= 100.0f) m_gpu_usage = static_cast<double>(gpu_u);
        }
      } catch (...) {
        // A failed optional provider must not stop basic system telemetry.
        m_sensor_error = L"Sensor provider failed during polling.";
        m_using_ohm = false;
        m_ohm.reset();
      }
    }
    m_gpu_source = m_gpu_usage >= 0 ? L"LibreHardwareMonitor" : L"Unavailable";
    if(!CanUseUnidentifiedGpuSensor(m_hardware_gpu_count)){m_gpu_temp=m_gpu_usage=-1;m_gpu_temperature_source=L"Unavailable (adapter identity ambiguous)";}
    PollPdhGpu();
    if (m_using_nvapi && ((m_gpu_requested && m_gpu_temp < 0) || m_gpu_usage < 0)) {
        PollNvApi();
    }
    if(!CanUseUnidentifiedGpuSensor(m_hardware_gpu_count)&&m_gpu_source==L"LibreHardwareMonitor")m_gpu_source=L"Unavailable (GPU identity ambiguous)";

}

void HardwareMonitor::PollNvApi() {
    if (!m_using_nvapi || m_nvGpuCount <= 0) return;

    int* primaryGpu = m_nvGpuHandles[0];
    // The legacy bridge/NVAPI handles have no per-adapter LUID. Only use the
    // sole NVIDIA handle when it unambiguously matches the active PDH adapter.
    if(m_nvGpuCount!=1||m_nvidia_gpu_count!=1)return;
    if(m_gpu_source==L"Windows GPU Engine counters (busiest physical engine)"){
        const auto key=GpuAdapterKey(m_active_gpu_engine);
        const auto vendor=m_gpu_vendors.find(key);
        if(vendor==m_gpu_vendors.end()||!CanUseNvidiaSensor(vendor->second,m_nvidia_gpu_count,m_nvGpuCount))return;
    }else{
        // PDH may have stopped after an integrated-GPU sample. A fallback
        // NVIDIA temperature must replace that old label as well as the value.
        m_gpu_prefix=L"GPU:";
        for(const auto& [key,vendor]:m_gpu_vendors)if(vendor==0x10DE){m_gpu_prefix=GetActiveGpuLabel(key);break;}
    }

    // Read GPU usage %
    if (m_gpu_usage < 0 && m_NvAPI_GPU_GetDynamicPstatesInfo) {
        // Public NVIDIA API, including its explicit domain-presence flag.
        // A zero-filled unsupported domain is not a measured idle GPU.
        NV_GPU_DYNAMIC_PSTATES_INFO usages{};
        usages.version=sizeof(usages)|(1u<<16);
        if (m_NvAPI_GPU_GetDynamicPstatesInfo(primaryGpu,&usages) == 0) {
            if (usages.utilization[0].present&&usages.utilization[0].percentage <= 100) {
                m_gpu_usage = static_cast<double>(usages.utilization[0].percentage);
                m_gpu_source = L"NVIDIA NVAPI";
            }
        }
    }

    // Read GPU temperature (°C)
    if (m_gpu_requested && m_gpu_temp < 0 && m_NvAPI_GPU_GetThermalSettings) {
        NV_GPU_THERMAL_SETTINGS_V2 settings{};
        settings.version = sizeof(NV_GPU_THERMAL_SETTINGS_V2) | (2 << 16);
        if (m_NvAPI_GPU_GetThermalSettings(primaryGpu, 15, &settings) == 0) {
            for(unsigned i=0;i<(std::min)(settings.count,3u);++i)if(settings.sensor[i].target==1&&settings.sensor[i].currentTemp>0&&settings.sensor[i].currentTemp<=150) {
                m_gpu_temp = static_cast<double>(settings.sensor[i].currentTemp);
                m_gpu_temperature_source=L"NVIDIA NVAPI (verified adapter)";
                break;
            }
        }
    }
}

} // namespace tmp
