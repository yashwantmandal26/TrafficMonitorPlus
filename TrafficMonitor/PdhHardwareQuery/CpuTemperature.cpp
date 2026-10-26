#include "stdafx.h"
#include "CpuTemperature.h"

CPdhCpuTemperature::CPdhCpuTemperature()
    : CPdhQuery(_T("\\Thermal Zone Information(*)\\High Precision Temperature"))
{
    if (!isInitialized)
    {
        fullCounterPath = _T("\\Thermal Zone Information(*)\\Temperature");
        m_high_precision = false;
        Initialize();
    }
}

bool CPdhCpuTemperature::GetCpuTemperature(float& temp)
{
    if (!isInitialized)
    {
        Initialize();
        if (!isInitialized)
            return false;
    }

    std::vector<CounterValueItem> values;
    if (QueryValues(values) && !values.empty())
    {
        float best_temp = -1.0f;
        for (const auto& item : values)
        {
            float c = 0.0f;
            if (m_high_precision)
                c = static_cast<float>((item.value / 10.0) - 273.15);
            else
                c = static_cast<float>(item.value - 273.15);

            if (c >= 20.0f && c <= 115.0f)
            {
                // Prefer the EC / CPU thermal zone if present
                std::wstring name_lower = item.name;
                for (auto& ch : name_lower) ch = towlower(ch);
                if (name_lower.find(L"ec") != std::wstring::npos ||
                    name_lower.find(L"tzrz") != std::wstring::npos ||
                    name_lower.find(L"cpu") != std::wstring::npos)
                {
                    best_temp = c;
                    break;
                }
                if (c > best_temp)
                {
                    best_temp = c;
                }
            }
        }

        if (best_temp > 0)
        {
            temp = best_temp;
            return true;
        }
    }
    return false;
}
