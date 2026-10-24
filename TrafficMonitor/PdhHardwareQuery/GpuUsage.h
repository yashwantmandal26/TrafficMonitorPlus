#pragma once
#include "PdhQuery.h"

// GPU utilization monitoring class
class CPdhGPUUsage : public CPdhQuery
{
public:
    CPdhGPUUsage();
    ~CPdhGPUUsage();

    // Get GPU utilization (0-100)
    // Return value: true=success, false=failure
    // Note: The first call usually returns false (PDH needs to be warmed up). It is recommended to call again at an interval of more than 200ms.
    bool GetGpuUsage(/*out*/ int& usage);

};
