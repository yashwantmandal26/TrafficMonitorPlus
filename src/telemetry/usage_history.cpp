#include "telemetry/usage_history.h"
#include "json.hpp"

namespace tmp {
using json = nlohmann::json;
std::string UsageHistory::LocalDate() {
    SYSTEMTIME now{};
    GetLocalTime(&now);
    return std::format("{:04}-{:02}-{:02}", now.wYear, now.wMonth, now.wDay);
}
static bool ValidDate(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') return false;
    for (size_t i = 0; i < date.size(); ++i)
        if (i != 4 && i != 7 && (date[i] < '0' || date[i] > '9')) return false;
    return std::chrono::year_month_day{std::chrono::year{std::stoi(date.substr(0, 4))},
        std::chrono::month{static_cast<unsigned>(std::stoi(date.substr(5, 2)))},
        std::chrono::day{static_cast<unsigned>(std::stoi(date.substr(8, 2)))}}.ok();
}
bool UsageHistory::Load(const fs::path& path) {
    m_days.clear();
    m_previous_download = m_previous_upload = 0;
    m_writable = true;
    m_status.clear();
    try {
        if (!fs::exists(path)) return true;
        std::ifstream file(path);
        auto data = json::parse(file);
        if (data.at("version") != 1 || !data.at("days").is_object()) throw std::runtime_error("schema");
        std::map<std::string, UsageTotal> loaded;
        for (const auto& [date, value] : data.at("days").items()) {
            if (!ValidDate(date) || !value.at("download").is_number_unsigned() ||
                !value.at("upload").is_number_unsigned()) throw std::runtime_error("entry");
            loaded[date] = {value.at("download").get<uint64_t>(), value.at("upload").get<uint64_t>()};
        }
        m_days = std::move(loaded);
        return true;
    } catch (...) {
        // Preserve an unreadable history rather than overwriting it with zeros.
        m_writable = false;
        m_status = L"History file could not be read. Existing file preserved; current totals are session-only.";
        return false;
    }
}
bool UsageHistory::Save(const fs::path& path) {
    if (!m_writable) return false;
    try {
        fs::create_directories(path.parent_path());
        json days = json::object();
        for (const auto& [date, total] : m_days)
            days[date] = {{"download", total.download}, {"upload", total.upload}};
        auto temporary = path;
        temporary += L".tmp";
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        file << json{{"version", 1}, {"days", days}}.dump(2);
        file.flush();
        const bool written = file.good();
        file.close();
        if (!written || !MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("save");
        m_status.clear();
        return true;
    } catch (...) {
        m_status = L"History could not be saved. Check disk space and folder permissions.";
        return false;
    }
}
void UsageHistory::Observe(const std::string& date, uint64_t download, uint64_t upload) {
    if (!ValidDate(date)) return;
    const auto down = download >= m_previous_download ? download - m_previous_download : 0;
    const auto up = upload >= m_previous_upload ? upload - m_previous_upload : 0;
    m_previous_download = download;
    m_previous_upload = upload;
    auto& total = m_days[date];
    total.download += down;
    total.upload += up;
}
UsageTotal UsageHistory::Day(const std::string& date) const {
    auto found = m_days.find(date);
    return found == m_days.end() ? UsageTotal{} : found->second;
}
bool UsageHistory::ExportCsv(const fs::path& path) const {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) return false;
    file << "date,download_bytes,upload_bytes,combined_bytes\r\n";
    for (const auto& [date, total] : m_days)
        file << date << ',' << total.download << ',' << total.upload << ',' << total.Combined() << "\r\n";
    file.flush();
    return file.good();
}
UsageTotal UsageHistory::Month(const std::string& month) const {
    UsageTotal total;
    if (month.size() != 7) return total;
    for (const auto& [date, day] : m_days) {
        if (date.substr(0, 7) == month) { total.download += day.download; total.upload += day.upload; }
    }
    return total;
}
}
