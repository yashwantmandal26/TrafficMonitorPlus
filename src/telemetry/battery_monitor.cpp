// TrafficMonitorPlus - Battery & Power Monitor Implementation
#include "pch.h"
#include "telemetry/battery_monitor.h"
#include "telemetry/measurement_validation.h"

#include <winioctl.h>
#include <batclass.h>
#include <setupapi.h>
#include <devguid.h>

#pragma comment(lib, "setupapi.lib")

namespace tmp {

void BatteryMonitor::Init() {
    Poll();
}

void BatteryMonitor::Poll() {
    SYSTEM_POWER_STATUS sps{};
    if (!GetSystemPowerStatus(&sps)) {
        m_has_battery = false;
        m_percent = -1;
        m_wattage = std::numeric_limits<double>::quiet_NaN();
        return;
    }

    // 128 = No system battery, 255 = Unknown status
    if (sps.BatteryFlag == 128) {
        m_has_battery = false;
        m_percent = -1;
        m_wattage = std::numeric_limits<double>::quiet_NaN();
        return;
    }

    m_has_battery = true;
    m_percent = sps.BatteryLifePercent<=100?static_cast<int>(sps.BatteryLifePercent):-1;
    const bool was_charging = m_is_charging;
    m_is_charging = sps.BatteryFlag!=255&&(sps.ACLineStatus == 1) && (sps.BatteryFlag & 8); // charging flag

    const auto now = GetTickCount64();
    if (now - m_last_rate_query >= 5000 || was_charging != m_is_charging || m_last_rate_query == 0 || m_last_ac_status!=sps.ACLineStatus) {
        m_last_ac_status=sps.ACLineStatus;
        m_last_rate_query = now;
        QueryBatteryRate();
    }
}

void BatteryMonitor::QueryBatteryRate() {
    m_wattage = std::numeric_limits<double>::quiet_NaN();

    HDEVINFO hdev = SetupDiGetClassDevsW(
        &GUID_DEVCLASS_BATTERY, nullptr, nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
    );

    if (hdev == INVALID_HANDLE_VALUE) {
        return;
    }

    SP_DEVICE_INTERFACE_DATA did{};
    did.cbSize = sizeof(did);
    std::vector<BatteryRateSample> samples;

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(hdev, nullptr, &GUID_DEVCLASS_BATTERY, i, &did); ++i) {
        DWORD cbRequired = 0;
        SetupDiGetDeviceInterfaceDetailW(hdev, &did, nullptr, 0, &cbRequired, nullptr);
        if (cbRequired == 0) continue;

        std::vector<BYTE> buf(cbRequired);
        PSP_DEVICE_INTERFACE_DETAIL_DATA_W pdidd = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(buf.data());
        pdidd->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        if (!SetupDiGetDeviceInterfaceDetailW(hdev, &did, pdidd, cbRequired, &cbRequired, nullptr)) {
            continue;
        }

        HANDLE hBat = CreateFileW(
            pdidd->DevicePath, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
        );

        if (hBat == INVALID_HANDLE_VALUE){samples.push_back({pdidd->DevicePath});continue;}
        BatteryRateSample sample{pdidd->DevicePath};

        BATTERY_QUERY_INFORMATION bqi{};
        DWORD dwWait = 0;
        DWORD dwOut = 0;

        if (DeviceIoControl(hBat, IOCTL_BATTERY_QUERY_TAG, &dwWait, sizeof(dwWait),
                            &bqi.BatteryTag, sizeof(bqi.BatteryTag), &dwOut, nullptr)) {
            BATTERY_WAIT_STATUS bws{};
            bws.BatteryTag = bqi.BatteryTag;
            BATTERY_STATUS bs{};

            if (DeviceIoControl(hBat, IOCTL_BATTERY_QUERY_STATUS, &bws, sizeof(bws),
                                &bs, sizeof(bs), &dwOut, nullptr)) {
                // Absolute rate is milliwatts; relative rates have no watt unit.
                // If Rate is positive, charging. If negative, discharging.
                BATTERY_INFORMATION information{};
                bqi.InformationLevel = BatteryInformation;
                const bool information_valid=DeviceIoControl(hBat, IOCTL_BATTERY_QUERY_INFORMATION,
                    &bqi, sizeof(bqi), &information, sizeof(information), &dwOut, nullptr)!=FALSE;
                const bool absolute=information_valid&&!(information.Capabilities&BATTERY_CAPACITY_RELATIVE);
                if(information_valid)sample.system_battery=(information.Capabilities&BATTERY_SYSTEM_BATTERY)!=0;
                if (absolute && bs.Rate != BATTERY_UNKNOWN_RATE) {
                    double rate_mw = static_cast<double>(bs.Rate);
                    // Convert mW to Watts
                    sample.watts = rate_mw / 1000.0;
                }
            }
        }

        CloseHandle(hBat);
        samples.push_back(std::move(sample));
    }

    SetupDiDestroyDeviceInfoList(hdev);
    m_wattage=AggregateBatteryWatts(samples);
}

} // namespace tmp
