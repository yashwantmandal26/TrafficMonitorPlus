// TrafficMonitorPlus - Telemetry Engine
// Central coordinator that owns and polls all hardware and telemetry monitors.
#pragma once
#include "../pch.h"
#include "../types.h"
#include "network_monitor.h"
#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "battery_monitor.h"
#include "hardware_monitor.h"

namespace tmp {

class TelemetryEngine {
public:
    TelemetryEngine();
    ~TelemetryEngine() = default;

    TelemetryEngine(const TelemetryEngine&) = delete;
    TelemetryEngine& operator=(const TelemetryEngine&) = delete;

    void Init();
    void Poll();
    void SetPollInterval(int milliseconds) { m_hardware.SetInterval(milliseconds); }
    void SetTemperatureRequests(bool cpu, bool gpu) { m_hardware.SetTemperatureRequests(cpu,gpu); }
    void RequestAdapterRescan(){m_hardware.RequestAdapterRescan();}

    // Thread-safe access to the latest telemetry snapshot
    TelemetryData GetLatestData() const;
    std::wstring GetHardwareStatus() const { return m_hardware.GetStatus(); }

    // Filter which network adapter is monitored
    void SetAdapterFilter(const std::wstring& name);

    // Network adapter info
    std::wstring GetLocalIP() const;
    std::wstring GetAdapterName() const;
    std::vector<std::wstring> GetAvailableAdapters() const;



private:
    NetworkMonitor   m_network;
    CpuMonitor       m_cpu;
    MemoryMonitor    m_memory;
    BatteryMonitor   m_battery;
    HardwareMonitor  m_hardware;

    TelemetryData    m_latest;


    mutable std::mutex m_mutex;
};

} // namespace tmp
