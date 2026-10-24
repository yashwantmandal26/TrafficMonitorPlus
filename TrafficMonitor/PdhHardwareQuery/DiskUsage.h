#pragma once
#include "PdhQuery.h"

class CPdhDiskUsage : public CPdhQuery
{
public:
    CPdhDiskUsage();
    ~CPdhDiskUsage();

    // Get the utilization of the specified physical disk (0-100)
    // diskIndex: 0=first physical disk, 1=second...
    // Return value: true=success, false=failure (the first call usually fails and requires an interval of 200ms+ to retry)
    bool GetDiskUsage(int diskIndex, /*out*/ int& usage);

    // Get a list of physical disk names
    const std::vector<CString>& GetDiskNames() const { return m_diskNames; }

    //Find the serial number of a hard drive based on its name
    int FindDiskIndex(const std::wstring diskName);

    // Check if disk monitoring is available
    bool IsAvailable() const { return m_isAvailable; }

private:
    // Extract disk name from counter instance
    void ExtractDiskNames();

    // Deal with idle time
    int CalculateUtilization(double idleTime) const;

private:
    bool m_isAvailable{ false };
    std::vector<CString> m_diskNames;  // List of disk names, in order returned by PDH
};