#pragma once
#include "pch.h"

namespace tmp {
struct UsageTotal {
    uint64_t download = 0, upload = 0;
    uint64_t Combined() const { return download + upload; }
};
class UsageHistory {
public:
    bool Load(const fs::path& path);
    bool Save(const fs::path& path);
    bool ExportCsv(const fs::path& path) const;
    // Exact cumulative byte counters from this process's selected adapter.
    void Observe(const std::string& date, uint64_t download, uint64_t upload);
    UsageTotal Day(const std::string& date) const;
    UsageTotal Month(const std::string& month) const;
    const std::wstring& Status() const { return m_status; }
    static std::string LocalDate();
private:
    std::map<std::string, UsageTotal> m_days;
    uint64_t m_previous_download = 0, m_previous_upload = 0;
    bool m_writable = true;
    std::wstring m_status;
};
}
