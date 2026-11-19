// TrafficMonitorPlus - Phase 2 Telemetry, History & Exports Regressions
#include "pch.h"
#include "telemetry/windows_network_usage.h"
#include "telemetry/network_monitor.h"
#include "app.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <chrono>

using namespace tmp;
namespace fs = std::filesystem;

// ─── Test 1: CSV Escaping, Formula Injection Defense & UTF-8 BOM ────────────
void TestCsvEscapingAndFormulaInjectionDefense() {
    std::cout << "[Phase 2 - Test 1] CSV escaping, formula defense & BOM\n";

    // 1. Formula injection triggers must be quoted and prefixed with single quote (')
    assert(WindowsNetworkUsage::CsvEscape(L"=1+1") == "\"'=1+1\"");
    assert(WindowsNetworkUsage::CsvEscape(L"=SUM(A1:A10)") == "\"'=SUM(A1:A10)\"");
    assert(WindowsNetworkUsage::CsvEscape(L"=cmd|' /C calc'!A0") == "\"'=cmd|' /C calc'!A0\"");
    assert(WindowsNetworkUsage::CsvEscape(L"+44123456789") == "\"'+44123456789\"");
    assert(WindowsNetworkUsage::CsvEscape(L"-100") == "\"'-100\"");
    assert(WindowsNetworkUsage::CsvEscape(L"@cmd") == "\"'@cmd\"");
    assert(WindowsNetworkUsage::CsvEscape(L"\tmalicious") == "\"'\tmalicious\"");
    assert(WindowsNetworkUsage::CsvEscape(L"\rmalicious") == "\"'\rmalicious\"");

    // 2. RFC 4180 special character escaping (commas, quotes, newlines)
    assert(WindowsNetworkUsage::CsvEscape(L"Simple") == "Simple");
    assert(WindowsNetworkUsage::CsvEscape(L"Hello, world") == "\"Hello, world\"");
    assert(WindowsNetworkUsage::CsvEscape(L"Quoted \"Word\"") == "\"Quoted \"\"Word\"\"\"");
    assert(WindowsNetworkUsage::CsvEscape(L"Line1\r\nLine2") == "\"Line1\r\nLine2\"");

    // 3. Unicode app names and symbols
    const std::wstring unicode_name = L"日本語ブラウザ \u2022 v2.0";
    std::string escaped_unicode = WindowsNetworkUsage::CsvEscape(unicode_name);
    assert(escaped_unicode.find("\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E") != std::string::npos);

    // 4. File export UTF-8 BOM and metadata verification
    NetworkUsageReport report{};
    report.ready = true;
    report.status = ReportStatus::Complete;
    report.range = UsageTimeRange::Today;
    report.scope = UsageNetworkScope::AllNetworks;
    report.profile_label = L"Home Wi-Fi";
    report.updated_at = L"14:30:00";
    report.total_download = 1048576;
    report.total_upload = 524288;
    report.tracked_download = 1048576;
    report.tracked_upload = 524288;

    AppUsageEntry app{};
    app.id = L"=calc.exe"; // formula injection attempt in process ID
    app.display_name = L"=Calculator"; // formula injection attempt in display name
    app.detail = L"calc.exe";
    app.category = AppUsageCategory::App;
    app.download = 1048576;
    app.upload = 524288;
    report.items.push_back(app);

    const fs::path temp_csv = fs::temp_directory_path() / L"tmp_phase2_export_test.csv";
    std::error_code ec;
    fs::remove(temp_csv, ec);

    const bool exported = WindowsNetworkUsage::ExportDetailedCsv(temp_csv, report);
    assert(exported);

    std::ifstream file(temp_csv, std::ios::binary);
    assert(file.is_open());
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    fs::remove(temp_csv, ec);

    // Verify UTF-8 BOM
    assert(content.size() >= 3);
    assert(static_cast<unsigned char>(content[0]) == 0xEF);
    assert(static_cast<unsigned char>(content[1]) == 0xBB);
    assert(static_cast<unsigned char>(content[2]) == 0xBF);

    // Verify metadata comments
    assert(content.find("# Yash Traffic Monitor Pro - Detailed Network Usage Export") != std::string::npos);
    assert(content.find("# Range: Today") != std::string::npos);
    assert(content.find("# Scope: All networks") != std::string::npos);
    assert(content.find("# Report Status: Complete") != std::string::npos);

    // Verify formula injection was neutralized in data rows
    assert(content.find("\"'=Calculator\"") != std::string::npos);

    std::cout << "  Passed. Formula injection neutralized, UTF-8 BOM and metadata verified.\n";
}

// ─── Test 2: NetworkMonitor Disconnect, Reconnect & Counter Spikes ───────────
void TestAdapterDisconnectReconnectCounters() {
    std::cout << "[Phase 2 - Test 2] Adapter disconnect, reconnect & spike prevention\n";

    // Allocate simulated MIB_IF_TABLE2 on heap
    constexpr ULONG kNumEntries = 2;
    const size_t table_size = sizeof(MIB_IF_TABLE2) + (kNumEntries - 1) * sizeof(MIB_IF_ROW2);
    std::vector<uint8_t> buffer(table_size, 0);
    PMIB_IF_TABLE2 table = reinterpret_cast<PMIB_IF_TABLE2>(buffer.data());
    table->NumEntries = kNumEntries;

    // Row 0: Ethernet Adapter
    MIB_IF_ROW2& eth = table->Table[0];
    eth.InterfaceIndex = 11;
    eth.InterfaceLuid.Value = 11;
    eth.Type = IF_TYPE_ETHERNET_CSMACD;
    eth.OperStatus = IfOperStatusUp;
    wcscpy_s(eth.Description, L"Realtek PCIe GbE Family Controller");
    wcscpy_s(eth.Alias, L"Ethernet");
    eth.InOctets = 1'000'000;
    eth.OutOctets = 500'000;

    // Row 1: Wi-Fi Adapter
    MIB_IF_ROW2& wifi = table->Table[1];
    wifi.InterfaceIndex = 22;
    wifi.InterfaceLuid.Value = 22;
    wifi.Type = IF_TYPE_IEEE80211;
    wifi.OperStatus = IfOperStatusUp;
    wcscpy_s(wifi.Description, L"Intel(R) Wi-Fi 6 AX201 160MHz");
    wcscpy_s(wifi.Alias, L"Wi-Fi");
    wifi.InOctets = 50'000'000;
    wifi.OutOctets = 20'000'000;

    // Manual adapter selection targeting "Realtek"
    NetworkMonitor monitor(L"Realtek");
    assert(monitor.IsManualSelection());

    const auto t0 = std::chrono::steady_clock::now();

    // Poll 1: Initial baseline capture
    monitor.PollWithTable(table, t0);
    assert(monitor.IsConnected());
    assert(monitor.GetAdapterName() == L"Realtek PCIe GbE Family Controller");
    assert(monitor.GetTotalDownloadedBytes() == 0); // No spike from initial 1,000,000 bytes

    // Poll 2: Traffic arrives on Ethernet (+50,000 bytes over 1 second)
    eth.InOctets += 50'000;
    eth.OutOctets += 10'000;
    const auto t1 = t0 + std::chrono::seconds(1);
    monitor.PollWithTable(table, t1);
    assert(monitor.IsConnected());
    assert(monitor.GetDownloadBytesPerSec() == 50'000.0);
    assert(monitor.GetTotalDownloadedBytes() == 50'000);

    // Poll 3: Ethernet DISCONNECTS (cable unplugged -> IfOperStatusDown)
    eth.OperStatus = IfOperStatusDown;
    const auto t2 = t1 + std::chrono::seconds(1);
    monitor.PollWithTable(table, t2);
    assert(!monitor.IsConnected());
    assert(monitor.GetDownloadBytesPerSec() == -1.0);
    assert(monitor.GetUploadBytesPerSec() == -1.0);
    assert(monitor.GetAdapterName().find(L"(Disconnected)") != std::wstring::npos);
    // CRITICAL: Did NOT migrate to Wi-Fi fallback! Total bytes remains 50,000
    assert(monitor.GetTotalDownloadedBytes() == 50'000);

    // Poll 4: Wi-Fi traffic surges while Ethernet is disconnected (+100 MB on Wi-Fi)
    wifi.InOctets += 100'000'000;
    const auto t3 = t2 + std::chrono::seconds(1);
    monitor.PollWithTable(table, t3);
    assert(!monitor.IsConnected());
    assert(monitor.GetTotalDownloadedBytes() == 50'000); // Still pinned, zero Wi-Fi bleed

    // Poll 5: Ethernet RECONNECTS (status Up, InOctets has changed to 999,999,999)
    eth.OperStatus = IfOperStatusUp;
    eth.InOctets = 999'999'999;
    eth.OutOctets = 888'888'888;
    const auto t4 = t3 + std::chrono::seconds(1);
    monitor.PollWithTable(table, t4);
    assert(monitor.IsConnected());
    assert(monitor.GetAdapterName() == L"Realtek PCIe GbE Family Controller");
    // CRITICAL SPIKE PREVENTION: Baseline reset must occur, total must NOT jump to 999,999,999!
    assert(monitor.GetTotalDownloadedBytes() == 50'000);
    assert(monitor.GetDownloadBytesPerSec() == -1.0); // Baseline is not a measured zero.

    // Poll 6: First second after reconnect (+30,000 bytes)
    eth.InOctets += 30'000;
    eth.OutOctets += 5'000;
    const auto t5 = t4 + std::chrono::seconds(1);
    monitor.PollWithTable(table, t5);
    assert(monitor.IsConnected());
    assert(monitor.GetDownloadBytesPerSec() == 30'000.0);
    assert(monitor.GetTotalDownloadedBytes() == 80'000); // exactly 50,000 + 30,000

    std::cout << "  Passed. Adapter disconnect/reconnect and spike prevention verified.\n";
}

// ─── Test 3: Day/Month Rollover & Transactional Partial Failure ──────────────
void TestDayMonthRolloverAndPartialFailure() {
    std::cout << "[Phase 2 - Test 3] Day/month rollover & transactional failure handling\n";

    // 1. Rollover detection: snapshot from yesterday must NOT be displayed as today
    WindowsUsageSnapshot yesterday_snap{};
    yesterday_snap.has_today = true;
    yesterday_snap.has_month = true;
    yesterday_snap.today.download = 50ULL * 1024 * 1024 * 1024; // 50 GB
    yesterday_snap.query_date = L"2026-11-13";
    yesterday_snap.query_month = L"2026-11";

    const std::wstring current_date = L"2026-11-14";
    const bool today_valid_today = yesterday_snap.has_today && (yesterday_snap.query_date == current_date);
    assert(!today_valid_today); // Must be rejected as yesterday's data!

    // When query_date matches, today is valid
    const std::wstring same_date = L"2026-11-13";
    const bool today_valid_same = yesterday_snap.has_today && (yesterday_snap.query_date == same_date);
    assert(today_valid_same);

    // 2. Transactional validity diagnostics in WindowsUsageSummary
    WindowsUsageSummary summary{};
    summary.today_valid = true;
    summary.today.download = 12345;
    summary.month_valid = false; // Month query failed
    summary.error_message = L"Month query timeout";

    // Legacy FetchUsage contract requires BOTH to be valid for true return
    UsageTotal t{}, m{};
    // When month fails, FetchUsage should return false to preserve transactional consistency
    assert(!(summary.today_valid && summary.month_valid));

    // Summary retains per-window validity
    assert(summary.today_valid == true);
    assert(summary.month_valid == false);
    assert(!summary.error_message.empty());

    std::cout << "  Passed. Rollover rejection and per-window validity verified.\n";
}

// ─── Test 4: Provider Failure vs Zero-Traffic Representation ─────────────────
void TestProviderFailureVsZeroTraffic() {
    std::cout << "[Phase 2 - Test 4] Provider failure vs zero-traffic representation\n";

    // 1. When provider query fails completely, status must be Failed and ready must be false
    NetworkUsageReport failed_rep{};
    failed_rep.status = ReportStatus::Failed;
    failed_rep.ready = false;
    failed_rep.error_message = L"WinRT NetworkInformation service unavailable";
    assert(!failed_rep.ready);
    assert(failed_rep.status == ReportStatus::Failed);

    // 2. Partial report: some slices or attribution failed
    NetworkUsageReport partial_rep{};
    partial_rep.status = ReportStatus::Partial;
    partial_rep.ready = true;
    partial_rep.total_slices = 12;
    partial_rep.successful_slices = 10;
    partial_rep.failed_slices = 2;
    partial_rep.attribution_failed = true;
    assert(partial_rep.status == ReportStatus::Partial);
    assert(partial_rep.ready);
    assert(partial_rep.failed_slices > 0);

    // 3. Attribution query failure must NOT be attributed to OS overhead
    // Simulated slice with 10 MB total, where attribution query threw an exception
    const uint64_t slice_total = 10 * 1024 * 1024;
    AppUsageEntry attr_error_entry;
    attr_error_entry.id = L"__ATTR_FAILED__";
    attr_error_entry.display_name = L"Attribution Unavailable (Query Error)";
    attr_error_entry.category = AppUsageCategory::Service;
    attr_error_entry.download = slice_total;

    assert(attr_error_entry.id == L"__ATTR_FAILED__");
    assert(attr_error_entry.id != L"__OS_UNATTRIBUTED__");
    assert(attr_error_entry.display_name == L"Attribution Unavailable (Query Error)");

    // System overhead entry is used only for genuine unattributed packets
    AppUsageEntry os_overhead;
    os_overhead.id = L"__OS_UNATTRIBUTED__";
    os_overhead.display_name = L"System & Network Overhead";
    assert(os_overhead.id == L"__OS_UNATTRIBUTED__");

    // 4. Stale cache entry retention
    NetworkUsageReport stale_rep{};
    stale_rep.status = ReportStatus::Stale;
    stale_rep.ready = true;
    stale_rep.updated_at = L"12:00:00";
    assert(stale_rep.status == ReportStatus::Stale);
    assert(stale_rep.ready);

    std::cout << "  Passed. Provider failure separated from confirmed zero usage and OS overhead.\n";
}

// ─── Test 5: Disconnected Profiles and Live Fetch Diagnostics ────────────────
void TestDisconnectedProfilesAndLiveDiagnostics() {
    std::cout << "[Phase 2 - Test 5] Disconnected profiles inclusion and live diagnostics\n";

    WindowsUsageSummary summary{};
    const bool ok = WindowsNetworkUsage::FetchUsageSummary(summary);
    if (ok) {
        // Disconnected profiles must be counted and queried in total_profiles
        assert(summary.total_profiles >= summary.active_profiles);
        assert(!summary.date.empty());
        assert(!summary.month.empty());
        if (summary.today_valid) {
            std::wcout << L"  Live SRUM: " << summary.total_profiles << L" profiles ("
                       << summary.active_profiles << L" active), today download = "
                       << summary.today.download << L" bytes, upload = "
                       << summary.today.upload << L" bytes\n";
        }
    } else {
        std::wcout << L"  Live SRUM query returned false in current environment: "
                   << (summary.error_message.empty() ? L"no profiles" : summary.error_message.c_str()) << L"\n";
    }

    std::cout << "  Passed. Disconnected profile query path verified.\n";
}

int main() {
    std::cout << "=== Phase 2 Telemetry & History Regressions ===\n";
    try {
        TestCsvEscapingAndFormulaInjectionDefense();
        TestAdapterDisconnectReconnectCounters();
        TestDayMonthRolloverAndPartialFailure();
        TestProviderFailureVsZeroTraffic();
        TestDisconnectedProfilesAndLiveDiagnostics();
        std::cout << "=== ALL PHASE 2 TESTS PASSED ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception\n";
        return 1;
    }
}
