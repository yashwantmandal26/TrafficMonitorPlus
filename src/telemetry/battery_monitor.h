// TrafficMonitorPlus - Battery & Power Monitor
#pragma once
#include "pch.h"
#include <limits>

namespace tmp {

class BatteryMonitor {
public:
    BatteryMonitor() = default;
    ~BatteryMonitor() = default;

    void Init();
    void Poll();

    bool HasBattery() const { return m_has_battery; }
    bool IsCharging() const { return m_is_charging; }
    int GetBatteryPercent() const { return m_percent; }
    double GetWattage() const { return m_wattage; }

private:
    void QueryBatteryRate();

    bool   m_has_battery = false;
    bool   m_is_charging = false;
    int    m_percent = -1;
    double m_wattage = std::numeric_limits<double>::quiet_NaN();
    ULONGLONG m_last_rate_query = 0;
    BYTE m_last_ac_status=255;
};

} // namespace tmp
