#pragma once
#include "PdhQuery.h"

class CPdhCpuTemperature : public CPdhQuery
{
public:
    CPdhCpuTemperature();
    bool GetCpuTemperature(float& temp);

private:
    bool m_high_precision{ true };
};
