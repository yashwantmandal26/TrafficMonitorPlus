// TrafficMonitorPlus - Windows Native Network Usage Tracker
#pragma once
#include "pch.h"
#include "telemetry/usage_history.h"

namespace tmp {

enum class UsageTimeRange {
    LastHour = 0,
    Today = 1,
    Last7Days = 2,
    ThisMonth = 3
};

enum class UsageNetworkScope {
    ActiveOnly = 0,
    AllNetworks = 1
};

enum class AppUsageCategory {
    App = 0,
    Service = 1
};

enum class ReportStatus {
    Loading = 0,
    Complete = 1,
    Partial = 2,
    Failed = 3,
    Stale = 4
};

struct WindowsUsageSummary {
    UsageTotal today{};
    UsageTotal this_month{};
    bool today_valid = false;
    bool month_valid = false;
    std::wstring date;   // "YYYY-MM-DD"
    std::wstring month;  // "YYYY-MM"
    std::wstring error_message;
    size_t total_profiles = 0;
    size_t active_profiles = 0;
    size_t queried_profiles = 0;
    size_t failed_profiles = 0;
};

struct UsageBucket {
    std::wstring label;       // Short axis label: "00:00", "01:25", "11 Nov", "05"
    std::wstring range_label; // Full interval: "00:00 - 01:00", "Today (11 Nov)"
    uint64_t download = 0;
    uint64_t upload = 0;
    uint64_t Combined() const { return download + upload; }
};

struct AppUsageEntry {
    std::wstring id;
    std::wstring display_name;
    std::wstring detail;
    AppUsageCategory category = AppUsageCategory::App;
    uint64_t download = 0;
    uint64_t upload = 0;
    std::vector<UsageBucket> timeline;
    std::wstring peak_when;
    std::wstring last_when;
    uint64_t Combined() const { return download + upload; }
};

struct NetworkUsageReport {
    UsageTimeRange range = UsageTimeRange::Today;
    UsageNetworkScope scope = UsageNetworkScope::ActiveOnly;
    ReportStatus status = ReportStatus::Loading;
    std::wstring profile_label;
    UsageTotal today_summary{};
    UsageTotal month_summary{};
    bool today_summary_valid=false,month_summary_valid=false;
    uint64_t total_download = 0;
    uint64_t total_upload = 0;
    uint64_t tracked_download = 0;
    uint64_t tracked_upload = 0;
    std::vector<UsageBucket> timeline;
    std::vector<AppUsageEntry> items;
    std::vector<std::wstring> active_services;
    std::wstring updated_at;
    std::wstring query_start_str;
    std::wstring query_end_str;
    std::wstring last_successful_update;
    std::wstring error_message;
    size_t total_slices = 0;
    size_t successful_slices = 0;
    size_t failed_slices = 0;
    bool attribution_supported = true;
    bool attribution_failed = false;
    uint64_t unattributed_download = 0;
    uint64_t unattributed_upload = 0;
    uint64_t query_error_bytes = 0;
    bool ready = false;
};

class WindowsNetworkUsage {
public:
    // Fetches cumulative network usage recorded directly by Windows OS for Today and This Month.
    static bool FetchUsage(UsageTotal& today, UsageTotal& this_month);

    // Extended cumulative usage fetch reporting per-window validity and profile diagnostics.
    static bool FetchUsageSummary(WindowsUsageSummary& out_summary);

    // Queries Windows SRUM per-app/per-service usage and time-bucketed ("when") history.
    static bool FetchDetailedUsage(UsageTimeRange range, UsageNetworkScope scope, NetworkUsageReport& out_report);

    // Resolves a raw SRUM attribution identifier into a user-friendly name and App/Service category.
    static AppUsageEntry ClassifyAndResolveApp(const std::wstring& raw_id);

    // Lists currently active Windows Services holding TCP/UDP sockets.
    static std::vector<std::wstring> GetActiveNetworkServices();

    // Safely escapes and sanitizes CSV fields, defending against formula injection (=, +, -, @, \t, \r).
    static std::string CsvEscape(const std::wstring& value);

    // Exports app/service totals and their non-zero time buckets to CSV with UTF-8 BOM and metadata.
    static bool ExportDetailedCsv(const fs::path& path, const NetworkUsageReport& report);
};

} // namespace tmp
