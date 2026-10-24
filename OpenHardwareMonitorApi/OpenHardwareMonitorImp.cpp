// This is the main DLL file.

#include "stdafx.h"

#include "OpenHardwareMonitorImp.h"
#include <vector>

namespace OpenHardwareMonitorApi
{
    static std::wstring error_message;

    //Convert CRL's String type to C++'s std::wstring type
    static std::wstring ClrStringToStdWstring(System::String^ str)
    {
        if (str == nullptr)
        {
            return std::wstring();
        }
        else
        {
            const wchar_t* chars = (const wchar_t*)(Runtime::InteropServices::Marshal::StringToHGlobalUni(str)).ToPointer();
            std::wstring os = chars;
            Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr((void*)chars));
            return os;
        }
    }


    std::shared_ptr<IOpenHardwareMonitor> CreateInstance()
    {
        std::shared_ptr<IOpenHardwareMonitor> pMonitor;
        try
        {
            MonitorGlobal::Instance()->Init();
            pMonitor = std::make_shared<COpenHardwareMonitor>();
        }
        catch (System::Exception^ e)
        {
            error_message = ClrStringToStdWstring(e->Message);
        }
        return pMonitor;
    }

    std::wstring GetErrorMessage()
    {
        return error_message;
    }

    float COpenHardwareMonitor::CpuTemperature()
    {
        return m_cpu_temperature;
    }

    float COpenHardwareMonitor::GpuTemperature()
    {
        if (m_gpu_nvidia_temperature >= 0)
            return m_gpu_nvidia_temperature;
        else if (m_gpu_ati_temperature >= 0)
            return m_gpu_ati_temperature;
        else
            return m_gpu_intel_temperature;
    }

    float COpenHardwareMonitor::HDDTemperature()
    {
        return m_hdd_temperature;
    }

    float COpenHardwareMonitor::MainboardTemperature()
    {
        return m_main_board_temperature;
    }

    float COpenHardwareMonitor::GpuUsage()
    {
        if (m_gpu_nvidia_usage >= 0)
            return m_gpu_nvidia_usage;
        else if (m_gpu_ati_usage >= 0)
            return m_gpu_ati_usage;
        else
            return m_gpu_intel_usage;
    }

    float COpenHardwareMonitor::CpuFreq()
    {
            return m_cpu_freq;
    }

    float COpenHardwareMonitor::CpuUsage()
    {
        return m_cpu_usage;
    }

    const std::map<std::wstring, float>& COpenHardwareMonitor::AllHDDTemperature()
    {
        return m_all_hdd_temperature;
    }

    const std::map<std::wstring, float>& COpenHardwareMonitor::AllCpuTemperature()
    {
        return m_all_cpu_temperature;
    }

    const std::map<std::wstring, float>& COpenHardwareMonitor::AllHDDUsage()
    {
        return m_all_hdd_usage;
    }

    void COpenHardwareMonitor::SetCpuEnable(bool enable)
    {
        MonitorGlobal::Instance()->computer->IsCpuEnabled = enable;
    }

    void COpenHardwareMonitor::SetGpuEnable(bool enable)
    {
        MonitorGlobal::Instance()->computer->IsGpuEnabled = enable;
    }

    void COpenHardwareMonitor::SetHddEnable(bool enable)
    {
        MonitorGlobal::Instance()->computer->IsStorageEnabled = enable;
    }

    void COpenHardwareMonitor::SetMainboardEnable(bool enable)
    {
        MonitorGlobal::Instance()->computer->IsMotherboardEnabled = enable;
    }

    bool COpenHardwareMonitor::GetCPUFreq(IHardware^ hardware, float& freq) {
        for (int i = 0; i < hardware->Sensors->Length; i++)
        {
            if (hardware->Sensors[i]->SensorType == SensorType::Clock)
            {
                String^ name = hardware->Sensors[i]->Name;
                if (name != L"Bus Speed")
                    m_all_cpu_clock[ClrStringToStdWstring(name)] = Convert::ToDouble(hardware->Sensors[i]->Value);
            }
        }
        float sum{};
        for (auto i : m_all_cpu_clock)
            sum += i.second;
        freq = sum / m_all_cpu_clock.size() / 1000.0;
        return true;
    }

    bool COpenHardwareMonitor::GetCpuUsage(IHardware^ hardware, float& cpu_usage)
    {
        for (int i = 0; i < hardware->Sensors->Length; i++)
        {
            if (hardware->Sensors[i]->SensorType == SensorType::Load)
            {
                String^ name = hardware->Sensors[i]->Name;
                if (name != L"CPU Total")
                {
                    cpu_usage = Convert::ToDouble(hardware->Sensors[i]->Value);
                    return true;
                }
            }
        }
        return false;
    }

    bool COpenHardwareMonitor::GetHardwareTemperature(IHardware^ hardware, float& temperature)
    {
        temperature = -1;
        std::vector<float> all_temperature;
        float core_temperature{ -1 };
        System::String^ temperature_name;
        switch (hardware->HardwareType)
        {
        case HardwareType::Cpu:
            temperature_name = L"Core Average";
            break;
        case HardwareType::GpuNvidia: case HardwareType::GpuAmd: case HardwareType::GpuIntel:
            temperature_name = L"GPU Core";
            break;
        default:
            break;
        }
        for (int i = 0; i < hardware->Sensors->Length; i++)
        {
            //Find the temperature sensor
            if (hardware->Sensors[i]->SensorType == SensorType::Temperature)
            {
                float cur_temperture = Convert::ToDouble(hardware->Sensors[i]->Value);
                all_temperature.push_back(cur_temperture);
                if (hardware->Sensors[i]->Name == temperature_name) //If the temperature sensor named temperature_name is found, save the temperature to core_temperature.
                    core_temperature = cur_temperture;
            }
        }
        if (core_temperature >= 0)
        {
            temperature = core_temperature;
            return true;
        }
        if (!all_temperature.empty())
        {
            //If there are multiple temperature sensors, take the average
            float sum{};
            for (auto i : all_temperature)
                sum += i;
            temperature = sum / all_temperature.size();
            return true;
        }
        //If the temperature sensor is not found, look for it in SubHardware
        for (int i = 0; i < hardware->SubHardware->Length; i++)
        {
            if (GetHardwareTemperature(hardware->SubHardware[i], temperature))
                return true;
        }
        return false;
    }

    bool COpenHardwareMonitor::GetCpuTemperature(IHardware^ hardware, float& temperature)
    {
        temperature = -1;
        m_all_cpu_temperature.clear();
        for (int i = 0; i < hardware->Sensors->Length; i++)
        {
            //Find the temperature sensor
            if (hardware->Sensors[i]->SensorType == SensorType::Temperature)
            {
                String^ name = hardware->Sensors[i]->Name;
                //Save the temperature of each CPU sensor
                m_all_cpu_temperature[ClrStringToStdWstring(name)] = Convert::ToDouble(hardware->Sensors[i]->Value);
            }
        }
        //Calculate average temperature
        if (!m_all_cpu_temperature.empty())
        {
            float sum{};
            for (const auto& item : m_all_cpu_temperature)
                sum += item.second;
            temperature = sum / m_all_cpu_temperature.size();
        }
        return temperature > 0;
    }

    bool COpenHardwareMonitor::GetGpuUsage(IHardware^ hardware, float& gpu_usage)
    {
        float usage_max = 0;
        for (int i = 0; i < hardware->Sensors->Length; i++)
        {
            //find load
            if (hardware->Sensors[i]->SensorType == SensorType::Load)
            {
                float cur_gpu_usage = Convert::ToDouble(hardware->Sensors[i]->Value);
                if (hardware->Sensors[i]->Name == L"GPU Core")
                {
                    gpu_usage = cur_gpu_usage;
                    return true;
                }

                //Calculate the maximum value
                if (cur_gpu_usage > usage_max)
                    usage_max = cur_gpu_usage;
            }
        }
        gpu_usage = usage_max;
        return true;
    }

    bool COpenHardwareMonitor::GetHddUsage(IHardware^ hardware, float& hdd_usage)
    {
        for (int i = 0; i < hardware->Sensors->Length; i++)
        {
            //find load
            if (hardware->Sensors[i]->SensorType == SensorType::Load)
            {
                if (hardware->Sensors[i]->Name == L"Total Activity")
                {
                    hdd_usage = Convert::ToDouble(hardware->Sensors[i]->Value);
                    return true;
                }
            }
        }
        return false;
    }

    COpenHardwareMonitor::COpenHardwareMonitor()
    {
        ResetAllValues();
    }

    COpenHardwareMonitor::~COpenHardwareMonitor()
    {
        MonitorGlobal::Instance()->UnInit();
    }

    void COpenHardwareMonitor::ResetAllValues()
    {
        m_cpu_temperature = -1;
        m_gpu_nvidia_temperature = -1;
        m_gpu_ati_temperature = -1;
        m_gpu_intel_temperature = -1;
        m_hdd_temperature = -1;
        m_main_board_temperature = -1;
        m_gpu_nvidia_usage = -1;
        m_gpu_ati_usage = -1;
        m_gpu_intel_usage = -1;
        m_all_hdd_temperature.clear();
        m_all_hdd_usage.clear();
        m_cpu_freq = -1;
        m_cpu_usage = -1;
    }

    void COpenHardwareMonitor::InsertValueToMap(std::map<std::wstring, float>& value_map, const std::wstring& key, float value)
    {
        auto iter = value_map.find(key);
        if (iter == value_map.end())
        {
            value_map[key] = value;
        }
        else
        {
            std::wstring key_exist = iter->first;
            size_t index = key_exist.rfind(L'#');   //Find whether a string contains the # sign
            if (index != std::wstring::npos)
            {
                //Get the number after the # sign and add 1 to it
                int num = _wtoi(key_exist.substr(index + 1).c_str());
                num++;
                key_exist = key_exist.substr(0, index + 1);
                key_exist += std::to_wstring(num);
            }
            else //If there is no # sign, add "#1" at the end
            {
                key_exist += L" #1";
            }
            value_map[key_exist] = value;
        }
    }

    void COpenHardwareMonitor::GetHardwareInfo()
    {
        ResetAllValues();
        error_message.clear();
        try
        {
            auto computer = MonitorGlobal::Instance()->computer;
            computer->Accept(MonitorGlobal::Instance()->updateVisitor);
            for (int i = 0; i < computer->Hardware->Count; i++)
            {
                //Find hardware type
                switch (computer->Hardware[i]->HardwareType)
                {
                case HardwareType::Cpu:
                    if (m_cpu_temperature < 0)
                        GetCpuTemperature(computer->Hardware[i], m_cpu_temperature);
                    if (m_cpu_freq < 0)
                        GetCPUFreq(computer->Hardware[i], m_cpu_freq);
                    if (m_cpu_usage < 0)
                        GetCpuUsage(computer->Hardware[i], m_cpu_usage);
                    break;
                case HardwareType::GpuNvidia:
                    if (m_gpu_nvidia_temperature < 0)
                        GetHardwareTemperature(computer->Hardware[i], m_gpu_nvidia_temperature);
                    if (m_gpu_nvidia_usage < 0)
                        GetGpuUsage(computer->Hardware[i], m_gpu_nvidia_usage);
                    break;
                case HardwareType::GpuAmd:
                    if (m_gpu_ati_temperature < 0)
                        GetHardwareTemperature(computer->Hardware[i], m_gpu_ati_temperature);
                    if (m_gpu_ati_usage < 0)
                        GetGpuUsage(computer->Hardware[i], m_gpu_ati_usage);
                    break;
                case HardwareType::GpuIntel:
                    if (m_gpu_intel_temperature < 0)
                        GetHardwareTemperature(computer->Hardware[i], m_gpu_intel_temperature);
                    if (m_gpu_intel_usage < 0)
                        GetGpuUsage(computer->Hardware[i], m_gpu_intel_usage);
                    break;
                case HardwareType::Storage:
                {
                    float cur_hdd_temperature = -1;
                    GetHardwareTemperature(computer->Hardware[i], cur_hdd_temperature);
                    //m_all_hdd_temperature[ClrStringToStdWstring(computer->Hardware[i]->Name)] = cur_hdd_temperature;
                    InsertValueToMap(m_all_hdd_temperature, ClrStringToStdWstring(computer->Hardware[i]->Name), cur_hdd_temperature);
                    float cur_hdd_usage = -1;
                    GetHddUsage(computer->Hardware[i], cur_hdd_usage);
                    //m_all_hdd_usage[ClrStringToStdWstring(computer->Hardware[i]->Name)] = cur_hdd_usage;
                    InsertValueToMap(m_all_hdd_usage, ClrStringToStdWstring(computer->Hardware[i]->Name), cur_hdd_usage);
                    if (m_hdd_temperature < 0)
                        m_hdd_temperature = cur_hdd_temperature;
                }
                break;
                case HardwareType::Motherboard:
                    if (m_main_board_temperature < 0)
                        GetHardwareTemperature(computer->Hardware[i], m_main_board_temperature);
                    break;
                default:
                    break;
                }
            }
        }
        catch (System::Exception^ e)
        {
            error_message = ClrStringToStdWstring(e->Message);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    MonitorGlobal::MonitorGlobal()
    {

    }

    MonitorGlobal::~MonitorGlobal()
    {

    }

    void MonitorGlobal::Init()
    {
        updateVisitor = gcnew UpdateVisitor();
        computer = gcnew Computer();
        computer->Open();
    }

    void MonitorGlobal::UnInit()
    {
        computer->Close();
    }

}
