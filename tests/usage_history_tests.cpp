#include "telemetry/usage_history.h"
#include <iostream>
int main() {
    int failures = 0;
    auto check = [&](bool valid, const char* name) { if (!valid) {std::cerr << name << '\n'; ++failures;} };
    const auto folder = fs::temp_directory_path() / (L"TrafficMonitor-usage-tests-" + std::to_wstring(GetCurrentProcessId()));
    const auto path = folder / L"usage.json";
    fs::create_directories(folder);
    tmp::UsageHistory history;
    check(history.Load(path), "new history");
    history.Observe("2026-10-31", 1000, 200);
    history.Observe("2026-10-31", 1500, 250);
    check(history.Day("2026-10-31").download == 1500 && history.Day("2026-10-31").upload == 250, "exact counters");
    history.Observe("2026-11-01", 1800, 350);
    history.Observe("2026-11-02", 2000, 400);
    check(history.Month("2026-11").Combined() == 650 && history.Month("2026-10").Combined() == 1750, "day/month rollover");
    history.Observe("2026-11-02", 10, 10);
    check(history.Month("2026-11").Combined() == 650, "reset does not inflate totals");
    check(history.Save(path), "atomic save");
    tmp::UsageHistory restarted;
    check(restarted.Load(path), "reload");
    restarted.Observe("2026-11-02", 100, 25);
    check(restarted.Month("2026-11").Combined() == 775, "new session and persisted totals");
    const auto csv = folder / L"history.csv";
    check(restarted.ExportCsv(csv), "CSV export");
    { std::ifstream file(csv); std::string header, first; std::getline(file, header); std::getline(file, first);
      check(header.find("date,download_bytes,upload_bytes,combined_bytes") == 0 && first.find("2026-10-31,1500,250,1750") == 0, "CSV uses exact separate and combined byte totals"); }
    restarted.Observe("2026-02-31", 150, 50);
    check(restarted.Day("2026-02-31").Combined() == 0, "invalid date");
    { std::ofstream output(path); output << "{broken"; }
    check(!restarted.Load(path) && !restarted.Save(path), "protect corrupt history");
    std::ifstream input(path);
    std::string content; input >> content; input.close();
    check(content == "{broken", "original file preserved");
    fs::remove_all(folder);
    return failures ? 1 : 0;
}
