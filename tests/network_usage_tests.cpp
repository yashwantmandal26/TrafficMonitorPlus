#include "telemetry/windows_network_usage.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main() {
    int failures = 0;

    // 1. Classification & friendly resolution tests
    {
        auto empty_svc = tmp::WindowsNetworkUsage::ClassifyAndResolveApp(L"");
        if (empty_svc.category != tmp::AppUsageCategory::Service ||
            empty_svc.display_name != L"Windows Background Services") {
            std::cerr << "Failed empty ID classification\n";
            ++failures;
        }

        auto kernel = tmp::WindowsNetworkUsage::ClassifyAndResolveApp(L"System");
        if (kernel.category != tmp::AppUsageCategory::Service ||
            kernel.display_name != L"Windows Kernel & System") {
            std::cerr << "Failed System classification\n";
            ++failures;
        }

        auto win_upd = tmp::WindowsNetworkUsage::ClassifyAndResolveApp(L"C:\\Windows\\System32\\MoUsoCoreWorker.exe");
        if (win_upd.category != tmp::AppUsageCategory::Service || win_upd.display_name.empty()) {
            std::cerr << "Failed MoUsoCoreWorker.exe classification\n";
            ++failures;
        }

        auto app = tmp::WindowsNetworkUsage::ClassifyAndResolveApp(L"C:\\Program Files\\CustomApp\\MyBrowser.exe");
        if (app.category != tmp::AppUsageCategory::App || app.display_name != L"MyBrowser") {
            std::cerr << "Failed exe app classification\n";
            ++failures;
        }

        auto svchost = tmp::WindowsNetworkUsage::ClassifyAndResolveApp(L"C:\\Windows\\System32\\svchost.exe");
        if (svchost.category != tmp::AppUsageCategory::Service) {
            std::cerr << "Failed svchost classification\n";
            ++failures;
        }

        auto shell_pkg = tmp::WindowsNetworkUsage::ClassifyAndResolveApp(L"Microsoft.Windows.Search_cw5n1h2txyewy");
        if (shell_pkg.category != tmp::AppUsageCategory::Service || shell_pkg.display_name.empty()) {
            std::cerr << "Failed shell package classification\n";
            ++failures;
        }
    }

    // 2. Active OS network services inspection
    {
        auto services = tmp::WindowsNetworkUsage::GetActiveNetworkServices();
        for (const auto& s : services) {
            if (s.empty()) {
                std::cerr << "Empty active service label returned\n";
                ++failures;
                break;
            }
        }
    }

    // 3. Detailed Windows SRUM queries (1h + Today)
    {
        tmp::NetworkUsageReport report_1h{};
        if (tmp::WindowsNetworkUsage::FetchDetailedUsage(
                tmp::UsageTimeRange::LastHour, tmp::UsageNetworkScope::ActiveOnly, report_1h)) {
            if (!report_1h.ready || report_1h.timeline.size() != 12) {
                std::cerr << "LastHour report timeline should have 12 5-minute buckets\n";
                ++failures;
            }
            for (size_t i = 0; i < report_1h.items.size(); ++i) {
                const auto& item = report_1h.items[i];
                if (item.Combined() != item.download + item.upload || item.timeline.size() != 12) {
                    std::cerr << "LastHour item timeline mismatch\n";
                    ++failures;
                    break;
                }
                if (i > 0 && report_1h.items[i - 1].Combined() < item.Combined()) {
                    std::cerr << "Items not sorted descending by total bytes\n";
                    ++failures;
                    break;
                }
            }
        }

        tmp::NetworkUsageReport report_today{};
        if (tmp::WindowsNetworkUsage::FetchDetailedUsage(
                tmp::UsageTimeRange::Today, tmp::UsageNetworkScope::ActiveOnly, report_today)) {
            std::cout << "Live Today query: ready=" << report_today.ready
                      << " status=" << static_cast<int>(report_today.status)
                      << " rows=" << report_today.items.size()
                      << " download=" << report_today.total_download << " upload=" << report_today.total_upload
                      << " attribution_failed=" << report_today.attribution_failed << "\n";
            if (!report_today.ready || report_today.timeline.empty() || report_today.timeline.size() > 24) {
                std::cerr << "Today report timeline bucket count out of bounds\n";
                ++failures;
            }
        } else {
            std::cout << "Live Today query unavailable in this session; app/service totals could not be verified.\n";
        }
    }

    // 4. CSV export verification
    {
        tmp::NetworkUsageReport report{};
        report.ready = true;
        report.range = tmp::UsageTimeRange::Today;
        report.scope = tmp::UsageNetworkScope::ActiveOnly;
        report.profile_label = L"Mirage";
        report.timeline = {
            { L"00:00", L"00:00 - 01:00", 1024ULL * 1024ULL, 256ULL * 1024ULL },
            { L"01:00", L"01:00 - 02:00", 2048ULL * 1024ULL, 512ULL * 1024ULL }
        };

        tmp::AppUsageEntry browser;
        browser.id = L"brave.exe";
        browser.display_name = L"Brave Browser";
        browser.detail = L"brave.exe";
        browser.category = tmp::AppUsageCategory::App;
        browser.download = 2500000;
        browser.upload = 300000;
        browser.peak_when = L"01:00 - 02:00";
        browser.last_when = L"01:00 - 02:00";
        browser.timeline = {
            { L"00:00", L"00:00 - 01:00", 500000, 100000 },
            { L"01:00", L"01:00 - 02:00", 2000000, 200000 }
        };

        tmp::AppUsageEntry svc;
        svc.id = L"";
        svc.display_name = L"Windows Background Services";
        svc.detail = L"svchost";
        svc.category = tmp::AppUsageCategory::Service;
        svc.download = 100000;
        svc.upload = 50000;
        svc.peak_when = L"00:00 - 01:00";
        svc.last_when = L"00:00 - 01:00";
        svc.timeline = {
            { L"00:00", L"00:00 - 01:00", 100000, 50000 },
            { L"01:00", L"01:00 - 02:00", 0, 0 }
        };

        report.items = { browser, svc };

        const fs::path temp_csv = fs::temp_directory_path() / L"tmp_network_usage_test.csv";
        std::error_code ec;
        fs::remove(temp_csv, ec);

        if (!tmp::WindowsNetworkUsage::ExportDetailedCsv(temp_csv, report)) {
            std::cerr << "ExportDetailedCsv failed\n";
            ++failures;
        } else {
            std::ifstream in(temp_csv, std::ios::binary);
            std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            in.close();
            if (content.find("section,range,scope,name,detail,category,when_window,last_active,download_bytes,upload_bytes,combined_bytes") == std::string::npos ||
                content.find("overall_timeline,Today,Mirage,All Network Traffic") == std::string::npos ||
                content.find("app_summary,Today,Mirage,Brave Browser,brave.exe,App,01:00 - 02:00,01:00 - 02:00,2500000,300000,2800000") == std::string::npos ||
                content.find("app_timeline,Today,Mirage,Windows Background Services,svchost,Service,00:00 - 01:00,00:00 - 01:00,100000,50000,150000") == std::string::npos) {
                std::cerr << "ExportDetailedCsv output missing expected rows:\n" << content << "\n";
                ++failures;
            }
            fs::remove(temp_csv, ec);
        }
    }

    if (failures) {
        std::cerr << failures << " network usage test failures\n";
        return 1;
    }
    return 0;
}
