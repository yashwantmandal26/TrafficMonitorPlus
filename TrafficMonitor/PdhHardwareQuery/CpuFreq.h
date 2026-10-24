#pragma once
#include <Pdh.h>
#include <PdhMsg.h>
#include "PdhQuery.h"

class CPdhCpuFreq : public CPdhQuery
{
public:
    CPdhCpuFreq();

    // Call this function to get the CPU frequency.
    // Note: The value can only be obtained after calling this function for the second time. The time between the two calls should not be too short, preferably greater than 200 milliseconds.
    bool GetCpuFreq(float& freq);

private:
    float max_cpu_freq = 0;
};
