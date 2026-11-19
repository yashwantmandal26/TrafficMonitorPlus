// TrafficMonitorPlus - Telemetry Engine Implementation
#include "pch.h"
#include "telemetry/telemetry_engine.h"

namespace tmp {

TelemetryEngine::TelemetryEngine()
    : m_network()
{
}

void TelemetryEngine::Init() {
    m_network.Init();
    m_cpu.Init();
    m_memory.Poll();
    m_battery.Init();
    m_hardware.Init();
}

void TelemetryEngine::Poll() {
    m_network.Poll();
    m_cpu.Poll();
    m_memory.Poll();
    m_battery.Poll();
    m_hardware.Poll();

    std::lock_guard<std::mutex> lock(m_mutex);

    m_latest.download_bytes_per_sec = m_network.GetDownloadBytesPerSec();
    m_latest.upload_bytes_per_sec   = m_network.GetUploadBytesPerSec();
    m_latest.total_session_download = m_network.GetTotalDownloadedBytes();
    m_latest.total_session_upload   = m_network.GetTotalUploadedBytes();

    m_latest.cpu_usage_percent = m_cpu.GetUsagePercent();
    m_latest.cpu_freq_ghz      = m_cpu.GetFrequencyGHz();

    m_latest.ram_usage_percent = m_memory.GetUsagePercent();
    m_latest.ram_used_bytes    = m_memory.GetUsedBytes();
    m_latest.ram_total_bytes   = m_memory.GetTotalBytes();

    // Hardware sensors (OpenHardwareMonitor or NVAPI)
    const auto hardware = m_hardware.GetReadout();
    m_latest.cpu_temp_celsius=hardware.cpu_temperature;
    m_latest.gpu_usage_percent=hardware.gpu_usage;
    m_latest.gpu_temp_celsius=hardware.gpu_temperature;
    m_latest.gpu_prefix        = hardware.gpu_prefix;

    // Battery
    m_latest.battery_wattage   = m_battery.GetWattage();
    m_latest.battery_percent   = m_battery.GetBatteryPercent();

    // Network ping & quality

    // Display

    m_latest.timestamp = std::chrono::steady_clock::now();
}

TelemetryData TelemetryEngine::GetLatestData() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_latest;
}

void TelemetryEngine::SetAdapterFilter(const std::wstring& name) {
    m_network.SetAdapterFilter(name);
}

std::wstring TelemetryEngine::GetLocalIP() const {
    const_cast<NetworkMonitor&>(m_network).UpdateLocalIP();
    return m_network.GetLocalIP();
}

std::wstring TelemetryEngine::GetAdapterName() const {
    return m_network.GetAdapterName();
}

std::vector<std::wstring> TelemetryEngine::GetAvailableAdapters() const {
    return m_network.GetAvailableAdapters();
}

} // namespace tmp
