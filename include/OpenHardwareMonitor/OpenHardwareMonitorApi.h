#pragma once
#include <memory>
#include "OpenHardwareMonitorGlobal.h"
#include <map>
#include <string>

namespace OpenHardwareMonitorApi
{
    class IOpenHardwareMonitor
    {
    public:
        virtual void GetHardwareInfo() = 0;     //Get hardware information once
        virtual float CpuTemperature() = 0;     //Returns the obtained CPU temperature
        virtual float GpuTemperature() = 0;     //Returns the obtained GPU temperature
        virtual float HDDTemperature() = 0;     //Returns the obtained hard disk temperature
        virtual float MainboardTemperature() = 0;   //Returns the obtained motherboard temperature
        virtual float GpuUsage() = 0;           //Returns the obtained GPU utilization
        virtual float CpuFreq() = 0;            //Returns the obtained CPU frequency
        virtual float CpuUsage() = 0;           //Returns the obtained CPU utilization
        virtual const std::map<std::wstring, float>& AllHDDTemperature() = 0;   //Returns the temperature of all hard drives. The key of the map is the name of the hard disk, and the value is the temperature of the hard disk.
        virtual const std::map<std::wstring, float>& AllCpuTemperature() = 0;   //Returns the temperature of all CPUs (cores). The key of the map is the name of the CPU, and the value is the temperature of the hard disk.
        virtual const std::map<std::wstring, float>& AllHDDUsage() = 0;         //Returns the usage of all hard disks

        virtual void SetCpuEnable(bool enable) = 0;
        virtual void SetGpuEnable(bool enable) = 0;
        virtual void SetHddEnable(bool enable) = 0;
        virtual void SetMainboardEnable(bool enable) = 0;
    };

    OPENHARDWAREMONITOR_API std::shared_ptr<IOpenHardwareMonitor> CreateInstance();
    OPENHARDWAREMONITOR_API std::wstring GetErrorMessage();
}
