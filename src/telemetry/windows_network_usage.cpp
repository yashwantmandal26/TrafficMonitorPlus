// TrafficMonitorPlus - Windows Native Network Usage Tracker Implementation
#include "pch.h"
#include "telemetry/windows_network_usage.h"
#include "telemetry/usage_accounting.h"
#include "utils/string_utils.h"
#include <winver.h>
#include <winsvc.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Networking.Connectivity.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Management.Deployment.h>
#include <unordered_set>

namespace tmp {

namespace {

using DateTime = winrt::Windows::Foundation::DateTime;
using TimeSpan = winrt::Windows::Foundation::TimeSpan;

std::wstring ToLower(std::wstring text) {
    for (wchar_t& ch : text) {
        if (ch >= L'A' && ch <= L'Z') {
            ch = static_cast<wchar_t>(ch - L'A' + L'a');
        }
    }
    return text;
}

bool ContainsInsensitive(const std::wstring& haystack_lower, const wchar_t* needle_lower) {
    return haystack_lower.find(needle_lower) != std::wstring::npos;
}

DateTime FileTimeToWinRt(const FILETIME& ft) {
    ULARGE_INTEGER uli{};
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return DateTime{ TimeSpan{ static_cast<int64_t>(uli.QuadPart) } };
}

FILETIME WinRtToFileTime(const DateTime& dt) {
    ULARGE_INTEGER uli{};
    uli.QuadPart = static_cast<uint64_t>(dt.time_since_epoch().count());
    FILETIME ft{};
    ft.dwLowDateTime = uli.LowPart;
    ft.dwHighDateTime = uli.HighPart;
    return ft;
}

DateTime LocalSystemTimeToWinRt(const SYSTEMTIME& local_st) {
    SYSTEMTIME utc_st{};
    TzSpecificLocalTimeToSystemTime(nullptr, &local_st, &utc_st);
    FILETIME ft{};
    SystemTimeToFileTime(&utc_st, &ft);
    return FileTimeToWinRt(ft);
}

SYSTEMTIME WinRtToLocalSystemTime(const DateTime& dt) {
    FILETIME utc_ft = WinRtToFileTime(dt);
    SYSTEMTIME utc_st{}, local_st{};
    FileTimeToSystemTime(&utc_ft, &utc_st);
    SystemTimeToTzSpecificLocalTime(nullptr, &utc_st, &local_st);
    return local_st;
}

SYSTEMTIME ShiftLocalDays(SYSTEMTIME base, int delta_days) {
    base.wHour = 12;
    base.wMinute = 0;
    base.wSecond = 0;
    base.wMilliseconds = 0;
    FILETIME ft{};
    SystemTimeToFileTime(&base, &ft);
    ULARGE_INTEGER uli{};
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    constexpr int64_t kDayTicks = 86400LL * 10'000'000LL;
    uli.QuadPart = static_cast<uint64_t>(static_cast<int64_t>(uli.QuadPart) + static_cast<int64_t>(delta_days) * kDayTicks);
    ft.dwLowDateTime = uli.LowPart;
    ft.dwHighDateTime = uli.HighPart;
    SYSTEMTIME shifted{};
    FileTimeToSystemTime(&ft, &shifted);
    shifted.wHour = 0;
    shifted.wMinute = 0;
    shifted.wSecond = 0;
    shifted.wMilliseconds = 0;
    return shifted;
}

const wchar_t* MonthShortName(WORD month) {
    static constexpr const wchar_t* kMonths[] = {
        L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
        L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
    };
    return (month >= 1 && month <= 12) ? kMonths[month - 1] : L"";
}

DateTime LocalMidnight() {
    SYSTEMTIME local_st{};
    GetLocalTime(&local_st);
    local_st.wHour = 0;
    local_st.wMinute = 0;
    local_st.wSecond = 0;
    local_st.wMilliseconds = 0;
    return LocalSystemTimeToWinRt(local_st);
}

DateTime MonthStart() {
    SYSTEMTIME local_st{};
    GetLocalTime(&local_st);
    local_st.wDay = 1;
    local_st.wHour = 0;
    local_st.wMinute = 0;
    local_st.wSecond = 0;
    local_st.wMilliseconds = 0;
    return LocalSystemTimeToWinRt(local_st);
}

void EnsureWinRtApartment() {
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
    } catch (const winrt::hresult_error&) {
        // Already initialized on this thread (for example STA in UI/test threads).
    }
}

std::wstring ProfileIdentity(const winrt::Windows::Networking::Connectivity::ConnectionProfile& profile){
    std::wstring adapter;
    try{if(auto network=profile.NetworkAdapter())adapter=winrt::to_hstring(network.NetworkAdapterId()).c_str();}catch(...){}
    return UsageIdentity(adapter,profile.ProfileName().c_str());
}

std::wstring DevicePathToDrivePath(const std::wstring& raw_path) {
    if (raw_path.empty() || raw_path[0] != L'\\') return raw_path;
    static std::mutex s_drive_mutex;
    static std::vector<std::pair<std::wstring, std::wstring>> s_drive_map;
    static ULONGLONG s_last_refresh = 0;

    std::lock_guard<std::mutex> lock(s_drive_mutex);
    const ULONGLONG now_tick = GetTickCount64();
    if (s_drive_map.empty() || (now_tick - s_last_refresh) > 60000) {
        s_drive_map.clear();
        wchar_t drives[512]{};
        DWORD len = GetLogicalDriveStringsW(static_cast<DWORD>(std::size(drives) - 1), drives);
        for (const wchar_t* p = drives; *p != L'\0' && static_cast<size_t>(p - drives) < len; p += wcslen(p) + 1) {
            if (wcslen(p) >= 2 && p[1] == L':') {
                std::wstring letter(p, 2);
                wchar_t target[MAX_PATH]{};
                if (QueryDosDeviceW(letter.c_str(), target, MAX_PATH) > 0) {
                    s_drive_map.emplace_back(ToLower(target), letter);
                }
            }
        }
        s_last_refresh = now_tick;
    }

    const std::wstring lower = ToLower(raw_path);
    for (const auto& [device_prefix, drive_letter] : s_drive_map) {
        if (lower.size() > device_prefix.size() &&
            lower.compare(0, device_prefix.size(), device_prefix) == 0 &&
            lower[device_prefix.size()] == L'\\') {
            return drive_letter + raw_path.substr(device_prefix.size());
        }
    }
    return raw_path;
}

std::wstring ReadPeDescription(const std::wstring& file_path) {
    if (file_path.empty() || file_path[0] == L'\\') return {};
    DWORD dummy = 0;
    const DWORD ver_size = GetFileVersionInfoSizeW(file_path.c_str(), &dummy);
    if (ver_size == 0) return {};

    std::vector<BYTE> buf(ver_size);
    if (!GetFileVersionInfoW(file_path.c_str(), 0, ver_size, buf.data())) return {};

    struct LangCodePage { WORD lang; WORD codepage; };
    LangCodePage* translations = nullptr;
    UINT trans_len = 0;
    std::vector<LangCodePage> candidates;
    if (VerQueryValueW(buf.data(), L"\\VarFileInfo\\Translation", reinterpret_cast<void**>(&translations), &trans_len) && translations) {
        const UINT count = trans_len / sizeof(LangCodePage);
        for (UINT i = 0; i < count; ++i) candidates.push_back(translations[i]);
    }
    candidates.push_back({0x0409, 0x04B0});
    candidates.push_back({0x0409, 0x04E4});

    for (const auto& cp : candidates) {
        for (const auto* field : {L"FileDescription", L"ProductName"}) {
            const std::wstring query = std::format(L"\\StringFileInfo\\{:04x}{:04x}\\{}", cp.lang, cp.codepage, field);
            wchar_t* val = nullptr;
            UINT val_len = 0;
            if (VerQueryValueW(buf.data(), query.c_str(), reinterpret_cast<void**>(&val), &val_len) && val && val_len > 1) {
                std::wstring text(val);
                while (!text.empty() && iswspace(text.back())) text.pop_back();
                while (!text.empty() && iswspace(text.front())) text.erase(text.begin());
                if (!text.empty() && ToLower(text) != L"microsoft® windows® operating system") {
                    return text;
                }
            }
        }
    }
    return {};
}

std::wstring LookupUwpDisplayName(const std::wstring& pfn) {
    static std::mutex s_uwp_mutex;
    static std::unordered_map<std::wstring, std::wstring> s_pfn_names;
    static ULONGLONG s_last_scan = 0;

    std::lock_guard<std::mutex> lock(s_uwp_mutex);
    const ULONGLONG now_tick = GetTickCount64();
    if (s_pfn_names.empty() || (now_tick - s_last_scan) > 120000) {
        try {
            EnsureWinRtApartment();
            winrt::Windows::Management::Deployment::PackageManager pm;
            for (const auto& pkg : pm.FindPackagesForUser(L"")) {
                std::wstring family(pkg.Id().FamilyName().c_str());
                std::wstring display(pkg.DisplayName().c_str());
                if (!family.empty() && !display.empty()) {
                    s_pfn_names[ToLower(family)] = display;
                }
            }
        } catch (...) {}
        s_last_scan = now_tick;
    }

    const auto found = s_pfn_names.find(ToLower(pfn));
    if (found != s_pfn_names.end()) return found->second;

    // Fallback: strip publisher hash suffix (e.g. "OpenAI.Codex_2p2nqsd0c76g0" -> "OpenAI Codex")
    std::wstring clean = pfn;
    const size_t underscore = clean.rfind(L'_');
    if (underscore != std::wstring::npos && underscore > 0) {
        clean = clean.substr(0, underscore);
    }
    for (wchar_t& ch : clean) {
        if (ch == L'.') ch = L' ';
    }
    return clean;
}

std::wstring PrettifyExeName(const std::wstring& filename_lower, const std::wstring& raw_name) {
    if (filename_lower == L"mousocoreworker.exe") return L"Windows Update Worker";
    if (filename_lower == L"sihclient.exe") return L"Windows Service Healing";
    if (filename_lower == L"mpcmdrun.exe" || filename_lower == L"msmpeng.exe") return L"Microsoft Defender Antivirus";
    if (filename_lower == L"compattelrunner.exe") return L"Windows Compatibility Telemetry";
    if (filename_lower == L"taskhostw.exe") return L"Windows Task Host Services";
    if (filename_lower == L"smartscreen.exe") return L"Microsoft Defender SmartScreen";
    if (filename_lower == L"svchost.exe") return L"Windows Service Host";
    if (filename_lower == L"lsass.exe") return L"Windows Local Security Service";
    if (filename_lower == L"wuauclt.exe" || filename_lower == L"tiworker.exe") return L"Windows Update Modules";
    if (filename_lower == L"nvdisplay.container.exe" || filename_lower == L"nvngx_update.exe") return L"NVIDIA Services & Updates";
    if (filename_lower == L"braveupdate.exe") return L"Brave Update Service";
    if (filename_lower == L"microsoftedgeupdate.exe") return L"Microsoft Edge Update Service";

    std::wstring base = raw_name;
    const size_t dot = base.rfind(L'.');
    if (dot != std::wstring::npos && _wcsicmp(base.c_str() + dot, L".exe") == 0) {
        base = base.substr(0, dot);
    }
    if (ContainsInsensitive(ToLower(base), L"tor-") && ContainsInsensitive(ToLower(base), L"brave")) {
        return L"Brave Tor Network";
    }
    for (wchar_t& ch : base) {
        if (ch == L'_' || ch == L'-') ch = L' ';
    }
    bool cap_next = true;
    for (wchar_t& ch : base) {
        if (iswspace(ch)) {
            cap_next = true;
        } else if (cap_next) {
            if (ch >= L'a' && ch <= L'z') ch = static_cast<wchar_t>(ch - L'a' + L'A');
            cap_next = false;
        }
    }
    return base.empty() ? raw_name : base;
}

bool IsLikelyServiceBinary(const std::wstring& path_lower, const std::wstring& exe_lower) {
    if (exe_lower == L"mousocoreworker.exe" || exe_lower == L"sihclient.exe" ||
        exe_lower == L"compattelrunner.exe" || exe_lower == L"taskhostw.exe" ||
        exe_lower == L"smartscreen.exe" || exe_lower == L"werfault.exe" ||
        exe_lower == L"wermgr.exe" || exe_lower == L"lsass.exe" ||
        exe_lower == L"svchost.exe" || exe_lower == L"searchindexer.exe" ||
        exe_lower == L"backgroundtaskhost.exe" || exe_lower == L"runtimebroker.exe" ||
        exe_lower == L"wuauclt.exe" || exe_lower == L"tiworker.exe" ||
        exe_lower == L"trustedinstaller.exe" || exe_lower == L"mpcmdrun.exe" ||
        exe_lower == L"msmpeng.exe" || exe_lower == L"nissrv.exe" ||
        exe_lower == L"braveupdate.exe" || exe_lower == L"microsoftedgeupdate.exe" ||
        exe_lower == L"nvdisplay.container.exe" || exe_lower == L"nvngx_update.exe" ||
        exe_lower == L"rockstarservice.exe" || exe_lower == L"autoupdate.exe" ||
        exe_lower == L"updater.exe") {
        return true;
    }
    if (ContainsInsensitive(exe_lower, L"service") || ContainsInsensitive(exe_lower, L"updater") ||
        ContainsInsensitive(exe_lower, L"update")) {
        return true;
    }
    if (ContainsInsensitive(path_lower, L"\\windows\\")) {
        if (exe_lower == L"powershell.exe" || exe_lower == L"pwsh.exe" ||
            exe_lower == L"cmd.exe" || exe_lower == L"curl.exe" ||
            exe_lower == L"explorer.exe" || exe_lower == L"notepad.exe") {
            return false;
        }
        return true;
    }
    return false;
}

struct TimeSlice {
    DateTime start;
    DateTime end;
    std::wstring label;
    std::wstring range_label;
};

std::vector<TimeSlice> BuildTimeSlices(UsageTimeRange range, DateTime now_dt, const SYSTEMTIME& local_now) {
    std::vector<TimeSlice> slices;
    constexpr int64_t kMinuteTicks = 60LL * 10'000'000LL;
    constexpr int64_t kHourTicks = 3600LL * 10'000'000LL;

    if (range == UsageTimeRange::LastHour) {
        const int64_t now_ticks = now_dt.time_since_epoch().count();
        for (int i = 0; i < 12; ++i) {
            const int64_t s_ticks = now_ticks - static_cast<int64_t>(12 - i) * 5 * kMinuteTicks;
            const int64_t e_ticks = now_ticks - static_cast<int64_t>(11 - i) * 5 * kMinuteTicks;
            DateTime s{ TimeSpan{ s_ticks } };
            DateTime e{ TimeSpan{ e_ticks } };
            const SYSTEMTIME ls = WinRtToLocalSystemTime(s);
            const SYSTEMTIME le = WinRtToLocalSystemTime(e);
            TimeSlice ts;
            ts.start = s;
            ts.end = e;
            ts.label = std::format(L"{:02d}:{:02d}", ls.wHour, ls.wMinute);
            ts.range_label = std::format(L"{:02d}:{:02d} - {:02d}:{:02d}", ls.wHour, ls.wMinute, le.wHour, le.wMinute);
            slices.push_back(std::move(ts));
        }
        return slices;
    }

    if (range == UsageTimeRange::Today) {
        const DateTime midnight = LocalMidnight();
        const int64_t mid_ticks = midnight.time_since_epoch().count();
        const int64_t now_ticks = now_dt.time_since_epoch().count();
        const int hours = (std::max)(1, static_cast<int>(local_now.wHour) + 1);
        for (int h = 0; h < hours; ++h) {
            const int64_t s_ticks = mid_ticks + static_cast<int64_t>(h) * kHourTicks;
            if (s_ticks >= now_ticks && h > 0) break;
            const int64_t e_ticks = (std::min)(now_ticks, s_ticks + kHourTicks);
            if (e_ticks <= s_ticks) continue;
            DateTime s{ TimeSpan{ s_ticks } };
            DateTime e{ TimeSpan{ e_ticks } };
            const SYSTEMTIME ls = WinRtToLocalSystemTime(s);
            const SYSTEMTIME le = WinRtToLocalSystemTime(e);
            TimeSlice ts;
            ts.start = s;
            ts.end = e;
            ts.label = std::format(L"{:02d}:00", ls.wHour);
            ts.range_label = std::format(L"{:02d}:{:02d} - {:02d}:{:02d}", ls.wHour, ls.wMinute, le.wHour, le.wMinute);
            slices.push_back(std::move(ts));
        }
        return slices;
    }

    if (range == UsageTimeRange::Last7Days) {
        SYSTEMTIME today_mid = local_now;
        today_mid.wHour = 0; today_mid.wMinute = 0; today_mid.wSecond = 0; today_mid.wMilliseconds = 0;
        for (int offset = -6; offset <= 0; ++offset) {
            const SYSTEMTIME ds = ShiftLocalDays(today_mid, offset);
            const DateTime s = LocalSystemTimeToWinRt(ds);
            const DateTime e = (offset == 0) ? now_dt : LocalSystemTimeToWinRt(ShiftLocalDays(today_mid, offset + 1));
            if (e.time_since_epoch().count() <= s.time_since_epoch().count()) continue;
            TimeSlice ts;
            ts.start = s;
            ts.end = e;
            ts.label = std::format(L"{:02d} {}", ds.wDay, MonthShortName(ds.wMonth));
            ts.range_label = (offset == 0)
                ? std::format(L"Today ({:02d} {})", ds.wDay, MonthShortName(ds.wMonth))
                : ts.label;
            slices.push_back(std::move(ts));
        }
        return slices;
    }

    // ThisMonth: day 1 through local_now.wDay
    SYSTEMTIME month_day = local_now;
    month_day.wDay = 1;
    month_day.wHour = 0; month_day.wMinute = 0; month_day.wSecond = 0; month_day.wMilliseconds = 0;
    const int days = (std::max)(1, static_cast<int>(local_now.wDay));
    for (int d = 0; d < days; ++d) {
        const SYSTEMTIME ds = ShiftLocalDays(month_day, d);
        const DateTime s = LocalSystemTimeToWinRt(ds);
        const DateTime e = (d == days - 1) ? now_dt : LocalSystemTimeToWinRt(ShiftLocalDays(month_day, d + 1));
        if (e.time_since_epoch().count() <= s.time_since_epoch().count()) continue;
        TimeSlice ts;
        ts.start = s;
        ts.end = e;
        ts.label = std::format(L"{:02d}", ds.wDay);
        ts.range_label = (d == days - 1)
            ? std::format(L"Today ({:02d} {})", ds.wDay, MonthShortName(ds.wMonth))
            : std::format(L"{:02d} {}", ds.wDay, MonthShortName(ds.wMonth));
        slices.push_back(std::move(ts));
    }
    return slices;
}

DWORD QueryOwnerModule(MIB_TCPROW_OWNER_MODULE* row, PVOID buf, PDWORD sz) {
    return GetOwnerModuleFromTcpEntry(row, TCPIP_OWNER_MODULE_INFO_BASIC, buf, sz);
}
DWORD QueryOwnerModule(MIB_TCP6ROW_OWNER_MODULE* row, PVOID buf, PDWORD sz) {
    return GetOwnerModuleFromTcp6Entry(row, TCPIP_OWNER_MODULE_INFO_BASIC, buf, sz);
}
DWORD QueryOwnerModule(MIB_UDPROW_OWNER_MODULE* row, PVOID buf, PDWORD sz) {
    return GetOwnerModuleFromUdpEntry(row, TCPIP_OWNER_MODULE_INFO_BASIC, buf, sz);
}
DWORD QueryOwnerModule(MIB_UDP6ROW_OWNER_MODULE* row, PVOID buf, PDWORD sz) {
    return GetOwnerModuleFromUdp6Entry(row, TCPIP_OWNER_MODULE_INFO_BASIC, buf, sz);
}

template <typename TRow>
void CollectServiceTags(const TRow* rows, DWORD count, bool is_tcp,
                      std::unordered_map<std::wstring, std::pair<int, int>>& service_counts) {
    std::vector<BYTE> owner_buf(2048);
    for (DWORD i = 0; i < count; ++i) {
        TRow row_copy = rows[i];
        DWORD buf_size = static_cast<DWORD>(owner_buf.size());
        DWORD err = QueryOwnerModule(&row_copy, owner_buf.data(), &buf_size);
        if (err == ERROR_INSUFFICIENT_BUFFER && buf_size > owner_buf.size() && buf_size < 65536) {
            owner_buf.resize(buf_size);
            err = QueryOwnerModule(&row_copy, owner_buf.data(), &buf_size);
        }
        if (err != NO_ERROR) continue;

        const auto* info = reinterpret_cast<const TCPIP_OWNER_MODULE_BASIC_INFO*>(owner_buf.data());
        if (!info->pModulePath || info->pModulePath[0] == L'\0') continue;

        const std::wstring tag(info->pModulePath);
        if (tag.find(L'\\') != std::wstring::npos || _wcsicmp(tag.c_str(), L"System") == 0 ||
            _wcsicmp(tag.c_str(), L"Idle") == 0) {
            continue;
        }
        auto& counts = service_counts[tag];
        counts.first += 1;
        if constexpr (requires { row_copy.dwState; }) {
            if (is_tcp && row_copy.dwState == MIB_TCP_STATE_ESTAB) counts.second += 1;
        }
    }
}

} // namespace

std::string WindowsNetworkUsage::CsvEscape(const std::wstring& value) {
    std::string utf8 = WideToUtf8(value);

    // Formula injection defense:
    // If the cell starts with =, +, -, @, \t, or \r, spreadsheet applications (Excel, Calc)
    // may interpret it as a formula or command. Prepend a single quote (') to neutralize.
    bool needs_prefix = false;
    if (!utf8.empty()) {
        const char first = utf8.front();
        if (first == '=' || first == '+' || first == '-' || first == '@' || first == '\t' || first == '\r') {
            needs_prefix = true;
        }
    }

    const bool has_special = (utf8.find_first_of(",\"\r\n") != std::string::npos);
    if (!needs_prefix && !has_special) {
        return utf8;
    }

    std::string quoted = "\"";
    if (needs_prefix) {
        quoted += '\'';
    }
    for (char ch : utf8) {
        if (ch == '"') {
            quoted += "\"\"";
        } else {
            quoted += ch;
        }
    }
    quoted += '"';
    return quoted;
}

AppUsageEntry WindowsNetworkUsage::ClassifyAndResolveApp(const std::wstring& raw_id) {
    static std::mutex s_cache_mutex;
    static std::unordered_map<std::wstring, AppUsageEntry> s_cache;

    {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        const auto found = s_cache.find(raw_id);
        if (found != s_cache.end()) {
            AppUsageEntry copy;
            copy.id = found->second.id;
            copy.display_name = found->second.display_name;
            copy.detail = found->second.detail;
            copy.category = found->second.category;
            return copy;
        }
    }

    AppUsageEntry resolved;
    resolved.id = raw_id;

    if (raw_id.empty()) {
        resolved.display_name = L"Windows Background Services";
        resolved.detail = L"svchost & background OS services";
        resolved.category = AppUsageCategory::Service;
    } else if (_wcsicmp(raw_id.c_str(), L"System") == 0) {
        resolved.display_name = L"Windows Kernel & System";
        resolved.detail = L"System";
        resolved.category = AppUsageCategory::Service;
    } else if (_wcsnicmp(raw_id.c_str(), L"System\\", 7) == 0) {
        resolved.display_name = L"System (" + raw_id.substr(7) + L")";
        resolved.detail = raw_id;
        resolved.category = AppUsageCategory::Service;
    } else if (raw_id.find(L'\\') != std::wstring::npos || raw_id.find(L'/') != std::wstring::npos ||
               (raw_id.size() > 4 && _wcsicmp(raw_id.c_str() + raw_id.size() - 4, L".exe") == 0)) {
        const std::wstring win_path = DevicePathToDrivePath(raw_id);
        const size_t slash = win_path.find_last_of(L"\\/");
        const std::wstring exe_name = (slash == std::wstring::npos) ? win_path : win_path.substr(slash + 1);
        const std::wstring exe_lower = ToLower(exe_name);
        const std::wstring path_lower = ToLower(win_path);

        resolved.detail = exe_name.empty() ? win_path : exe_name;
        resolved.category = IsLikelyServiceBinary(path_lower, exe_lower) ? AppUsageCategory::Service : AppUsageCategory::App;

        std::wstring desc;
        if (resolved.category == AppUsageCategory::Service &&
            (exe_lower == L"mousocoreworker.exe" || exe_lower == L"sihclient.exe" ||
             exe_lower == L"compattelrunner.exe" || exe_lower == L"mpcmdrun.exe" ||
             exe_lower == L"msmpeng.exe")) {
            desc = PrettifyExeName(exe_lower, exe_name);
        } else {
            desc = ReadPeDescription(win_path);
        }

        if (desc.empty()) {
            if (exe_lower == L"language_server.exe" && ContainsInsensitive(path_lower, L"antigravity")) {
                desc = L"Antigravity Language Server";
            } else if (ContainsInsensitive(exe_lower, L"tor-") && ContainsInsensitive(path_lower, L"brave")) {
                desc = L"Brave Tor Network";
            } else {
                desc = PrettifyExeName(exe_lower, exe_name);
            }
        }
        resolved.display_name = desc;
    } else {
        // UWP / Microsoft Store PackageFamilyName
        resolved.display_name = LookupUwpDisplayName(raw_id);
        resolved.detail = raw_id;
        const std::wstring lower_id = ToLower(raw_id);
        const bool is_shell_svc =
            lower_id.rfind(L"microsoft.windows.", 0) == 0 ||
            lower_id.rfind(L"microsoftwindows.", 0) == 0 ||
            lower_id.rfind(L"microsoft.startexperiences", 0) == 0 ||
            lower_id.rfind(L"microsoft.lockapp", 0) == 0 ||
            lower_id.rfind(L"windows.immersivecontrolpanel", 0) == 0;
        if (lower_id.find(L"notepad") != std::wstring::npos ||
            lower_id.find(L"photos") != std::wstring::npos ||
            lower_id.find(L"windowsstore") != std::wstring::npos) {
            resolved.category = AppUsageCategory::App;
        } else {
            resolved.category = is_shell_svc ? AppUsageCategory::Service : AppUsageCategory::App;
        }
    }

    {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        if (s_cache.size() >= 1024) s_cache.clear();
        s_cache[raw_id] = resolved;
    }
    return resolved;
}

std::vector<std::wstring> WindowsNetworkUsage::GetActiveNetworkServices() {
    std::unordered_map<std::wstring, std::pair<int, int>> service_counts;

    auto scan_tcp = [&](ULONG af) {
        DWORD size = 0;
        if (GetExtendedTcpTable(nullptr, &size, FALSE, af, TCP_TABLE_OWNER_MODULE_ALL, 0) != ERROR_INSUFFICIENT_BUFFER || size == 0) {
            return;
        }
        std::vector<BYTE> buf(size);
        for (int attempt = 0; attempt < 3; ++attempt) {
            DWORD err = GetExtendedTcpTable(buf.data(), &size, FALSE, af, TCP_TABLE_OWNER_MODULE_ALL, 0);
            if (err == NO_ERROR) {
                if (af == AF_INET) {
                    const auto* table = reinterpret_cast<const MIB_TCPTABLE_OWNER_MODULE*>(buf.data());
                    CollectServiceTags(table->table, table->dwNumEntries, true, service_counts);
                } else {
                    const auto* table = reinterpret_cast<const MIB_TCP6TABLE_OWNER_MODULE*>(buf.data());
                    CollectServiceTags(table->table, table->dwNumEntries, true, service_counts);
                }
                break;
            }
            if (err != ERROR_INSUFFICIENT_BUFFER || size == 0) break;
            buf.resize(size);
        }
    };

    auto scan_udp = [&](ULONG af) {
        DWORD size = 0;
        if (GetExtendedUdpTable(nullptr, &size, FALSE, af, UDP_TABLE_OWNER_MODULE, 0) != ERROR_INSUFFICIENT_BUFFER || size == 0) {
            return;
        }
        std::vector<BYTE> buf(size);
        for (int attempt = 0; attempt < 3; ++attempt) {
            DWORD err = GetExtendedUdpTable(buf.data(), &size, FALSE, af, UDP_TABLE_OWNER_MODULE, 0);
            if (err == NO_ERROR) {
                if (af == AF_INET) {
                    const auto* table = reinterpret_cast<const MIB_UDPTABLE_OWNER_MODULE*>(buf.data());
                    CollectServiceTags(table->table, table->dwNumEntries, false, service_counts);
                } else {
                    const auto* table = reinterpret_cast<const MIB_UDP6TABLE_OWNER_MODULE*>(buf.data());
                    CollectServiceTags(table->table, table->dwNumEntries, false, service_counts);
                }
                break;
            }
            if (err != ERROR_INSUFFICIENT_BUFFER || size == 0) break;
            buf.resize(size);
        }
    };

    scan_tcp(AF_INET);
    scan_tcp(AF_INET6);
    scan_udp(AF_INET);
    scan_udp(AF_INET6);

    struct RankedService {
        std::wstring tag;
        std::wstring label;
        int established = 0;
        int total_sockets = 0;
    };
    std::vector<RankedService> ranked;
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    for (const auto& [tag, counts] : service_counts) {
        RankedService rs;
        rs.tag = tag;
        rs.total_sockets = counts.first;
        rs.established = counts.second;
        rs.label = tag;
        if (scm) {
            wchar_t disp[256]{};
            DWORD disp_len = static_cast<DWORD>(std::size(disp));
            if (GetServiceDisplayNameW(scm, tag.c_str(), disp, &disp_len) && disp[0] != L'\0') {
                rs.label = std::format(L"{} ({})", disp, tag);
            }
        }
        ranked.push_back(std::move(rs));
    }
    if (scm) CloseServiceHandle(scm);

    std::sort(ranked.begin(), ranked.end(), [](const RankedService& a, const RankedService& b) {
        if (a.established != b.established) return a.established > b.established;
        if (a.total_sockets != b.total_sockets) return a.total_sockets > b.total_sockets;
        return a.tag < b.tag;
    });

    std::vector<std::wstring> out;
    out.reserve(ranked.size());
    for (const auto& rs : ranked) out.push_back(rs.label);
    return out;
}

bool WindowsNetworkUsage::FetchUsageSummary(WindowsUsageSummary& out_summary) {
    out_summary={};
    SYSTEMTIME local{};GetLocalTime(&local);
    out_summary.date=std::format(L"{:04}-{:02}-{:02}",local.wYear,local.wMonth,local.wDay);
    out_summary.month=std::format(L"{:04}-{:02}",local.wYear,local.wMonth);
    try{
        EnsureWinRtApartment();
        using namespace winrt::Windows::Networking::Connectivity;
        const auto now=winrt::clock::now(),today_start=LocalMidnight(),month_start=MonthStart();
        NetworkUsageStates states{};states.Roaming=states.Shared=TriStates::DoNotCare;
        UsageWindowAccumulator today,month;
        std::set<std::wstring> identities;
        auto query=[&](const ConnectionProfile& profile){
            const auto key=ProfileIdentity(profile);
            if(!identities.insert(key).second)return;
            if(profile.GetNetworkConnectivityLevel()!=NetworkConnectivityLevel::None)++out_summary.active_profiles;
            bool failed=false;
            auto read=[&](auto start,UsageWindowAccumulator& window){
                try{
                    UsageTotal bytes{};
                    for(const auto& usage:profile.GetNetworkUsageAsync(start,now,DataUsageGranularity::Total,states).get()){
                        if(usage.BytesReceived()>UINT64_MAX-bytes.download||usage.BytesSent()>UINT64_MAX-bytes.upload)
                            throw std::overflow_error("Network byte total overflow");
                        bytes.download+=usage.BytesReceived();bytes.upload+=usage.BytesSent();
                    }
                    window.Add(key,bytes);
                }catch(...){failed=true;}
            };
            read(today_start,today);read(month_start,month);
            ++out_summary.queried_profiles;if(failed)++out_summary.failed_profiles;
        };
        for(const auto& profile:NetworkInformation::GetConnectionProfiles())query(profile);
        if(identities.empty())if(auto active=NetworkInformation::GetInternetConnectionProfile())query(active);
        out_summary.total_profiles=identities.size();
        out_summary.today=today.total;out_summary.this_month=month.total;
        // Partial profile results must not be presented as complete totals.
        out_summary.today_valid=!identities.empty()&&today.profiles.size()==identities.size();
        out_summary.month_valid=!identities.empty()&&month.profiles.size()==identities.size();
        if(out_summary.failed_profiles)out_summary.error_message=L"Some Windows network profiles could not be read; partial totals are not complete.";
        return !today.profiles.empty()||!month.profiles.empty();
    }catch(const std::exception& error){out_summary.error_message=Utf8ToWide(error.what());}
    catch(...){out_summary.error_message=L"Windows network usage is unavailable.";}
    return false;
}
bool WindowsNetworkUsage::FetchUsage(UsageTotal& today, UsageTotal& this_month) {
    WindowsUsageSummary summary{};
    if (FetchUsageSummary(summary)) {
        if (summary.today_valid) {
            today = summary.today;
        }
        if (summary.month_valid) {
            this_month = summary.this_month;
        }
        return summary.today_valid && summary.month_valid;
    }
    return false;
}

bool WindowsNetworkUsage::FetchDetailedUsage(UsageTimeRange range, UsageNetworkScope scope, NetworkUsageReport& out_report) {
    try {
        EnsureWinRtApartment();
        auto profiles = winrt::Windows::Networking::Connectivity::NetworkInformation::GetConnectionProfiles();
        if (profiles.Size() == 0) return false;

        const DateTime now = winrt::clock::now();
        SYSTEMTIME local_now{};
        GetLocalTime(&local_now);
        const DateTime today_start = LocalMidnight();
        const DateTime month_start = MonthStart();
        const auto slices = BuildTimeSlices(range, now, local_now);
        if (slices.empty()) return false;
        const DateTime range_start = slices.front().start;

        winrt::Windows::Networking::Connectivity::NetworkUsageStates states{};
        states.Roaming = winrt::Windows::Networking::Connectivity::TriStates::DoNotCare;
        states.Shared = winrt::Windows::Networking::Connectivity::TriStates::DoNotCare;

        NetworkUsageReport report;
        report.range = range;
        report.scope = scope;
        report.status = ReportStatus::Loading;
        report.timeline.resize(slices.size());
        for (size_t i = 0; i < slices.size(); ++i) {
            report.timeline[i].label = slices[i].label;
            report.timeline[i].range_label = slices[i].range_label;
        }

        using Profile = winrt::Windows::Networking::Connectivity::ConnectionProfile;
        std::vector<Profile> selected_profiles;
        std::vector<std::wstring> selected_names;
        std::set<std::wstring> profile_ids;

        UsageWindowAccumulator summary_today,summary_month;
        auto read_summary=[&](const Profile& profile,const std::wstring& key,DateTime start,UsageWindowAccumulator& result){
            try{
                UsageTotal bytes{};
                for(const auto& usage:profile.GetNetworkUsageAsync(start,now,winrt::Windows::Networking::Connectivity::DataUsageGranularity::Total,states).get()){
                    if(usage.BytesReceived()>UINT64_MAX-bytes.download||usage.BytesSent()>UINT64_MAX-bytes.upload)
                        throw std::overflow_error("Network byte total overflow");
                    bytes.download+=usage.BytesReceived();bytes.upload+=usage.BytesSent();
                }
                result.Add(key,bytes);
            }catch(...){}
        };
        auto select=[&](const Profile& profile){
            const auto key=ProfileIdentity(profile);
            if(!profile_ids.insert(key).second)return;
            selected_profiles.push_back(profile);selected_names.emplace_back(profile.ProfileName().c_str());
            // A failed month query must neither discard a valid day query nor
            // silently exclude this profile from the detailed traffic report.
            read_summary(profile,key,today_start,summary_today);
            read_summary(profile,key,month_start,summary_month);
        };
        for(const auto& profile:profiles){
            if(scope==UsageNetworkScope::AllNetworks||profile.GetNetworkConnectivityLevel()!=winrt::Windows::Networking::Connectivity::NetworkConnectivityLevel::None)
                select(profile);
        }
        if(selected_profiles.empty())if(auto active=winrt::Windows::Networking::Connectivity::NetworkInformation::GetInternetConnectionProfile())select(active);
        report.today_summary=summary_today.total;report.month_summary=summary_month.total;
        report.today_summary_valid=!profile_ids.empty()&&summary_today.profiles.size()==profile_ids.size();
        report.month_summary_valid=!profile_ids.empty()&&summary_month.profiles.size()==profile_ids.size();
        if (selected_profiles.empty()) return false;

        if (scope == UsageNetworkScope::AllNetworks) {
            report.profile_label = selected_names.size() == 1
                ? selected_names.front()
                : std::format(L"All networks ({})", selected_names.size());
        } else {
            report.profile_label = selected_names.front();
            for (size_t i = 1; i < selected_names.size(); ++i) {
                report.profile_label += L", " + selected_names[i];
            }
        }

        std::unordered_map<std::wstring, AppUsageEntry> aggregated;
        std::vector<uint64_t> bucket_attr_down(slices.size(), 0);
        std::vector<uint64_t> bucket_attr_up(slices.size(), 0);
        std::vector<uint64_t> bucket_attr_error_down(slices.size(), 0);
        std::vector<uint64_t> bucket_attr_error_up(slices.size(), 0);

        size_t total_slice_queries = slices.size() * selected_profiles.size();
        size_t successful_slice_queries = 0;
        size_t failed_slice_queries = 0;
        bool any_attr_attempted = false;
        bool any_attr_failed = false;

        for (const auto& p : selected_profiles) {
            for (size_t b = 0; b < slices.size(); ++b) {
                uint64_t slice_down = 0, slice_up = 0;
                bool got_total = false;
                try {
                    auto u_tot = p.GetNetworkUsageAsync(slices[b].start, slices[b].end,
                        winrt::Windows::Networking::Connectivity::DataUsageGranularity::Total, states).get();
                    got_total = true;
                    for (const auto& u : u_tot) {
                        slice_down += u.BytesReceived();
                        slice_up += u.BytesSent();
                    }
                    report.timeline[b].download += slice_down;
                    report.timeline[b].upload += slice_up;
                    report.total_download += slice_down;
                    report.total_upload += slice_up;
                    successful_slice_queries++;
                } catch (...) {
                    failed_slice_queries++;
                }

                if (!got_total || (slice_down == 0 && slice_up == 0)) {
                    continue;
                }

                any_attr_attempted = true;
                try {
                    auto att_list = p.GetAttributedNetworkUsageAsync(slices[b].start, slices[b].end, states).get();
                    std::vector<AttributedBytes> attribution;
                    for (const auto& u : att_list) {
                        attribution.push_back({u.AttributionId().c_str(),u.AttributionName().c_str(),{u.BytesReceived(),u.BytesSent()}});
                    }
                    // Validate the entire slice before committing any app row:
                    // a provider exception/duplicate must not leave partial rows
                    // alongside another full "query failed" copy of those bytes.
                    for(const auto& u:ValidateAttribution(attribution,{slice_down,slice_up})){
                        const uint64_t down = u.bytes.download;
                        const uint64_t up = u.bytes.upload;
                        if (down == 0 && up == 0) continue;

                        std::wstring raw_id=u.id;
                        std::wstring raw_name=u.name;
                        auto it = aggregated.find(raw_id);
                        if (it == aggregated.end()) {
                            AppUsageEntry entry = ClassifyAndResolveApp(raw_id);
                            if (entry.display_name.empty() && !raw_name.empty()) {
                                entry.display_name = raw_name;
                            }
                            entry.timeline.resize(slices.size());
                            for (size_t k = 0; k < slices.size(); ++k) {
                                entry.timeline[k].label = slices[k].label;
                                entry.timeline[k].range_label = slices[k].range_label;
                            }
                            it = aggregated.emplace(raw_id, std::move(entry)).first;
                        }
                        it->second.download += down;
                        it->second.upload += up;
                        it->second.timeline[b].download += down;
                        it->second.timeline[b].upload += up;

                        bucket_attr_down[b] += down;
                        bucket_attr_up[b] += up;
                        report.tracked_download += down;
                        report.tracked_upload += up;
                    }
                } catch (...) {
                    any_attr_failed = true;
                    bucket_attr_error_down[b] += slice_down;
                    bucket_attr_error_up[b] += slice_up;
                    report.query_error_bytes += (slice_down + slice_up);
                }
            }
        }

        // If no slice queries succeeded at all, do not report confirmed zero traffic
        if (successful_slice_queries == 0) {
            report.status = ReportStatus::Failed;
            report.ready = false;
            report.error_message = L"Unable to query network usage data from Windows.";
            out_report = std::move(report);
            return false;
        }

        report.total_slices = total_slice_queries;
        report.successful_slices = successful_slice_queries;
        report.failed_slices = failed_slice_queries;
        report.attribution_failed = any_attr_failed;
        report.attribution_supported = !any_attr_failed || (report.tracked_download + report.tracked_upload > 0);
        report.query_start_str = slices.front().range_label;
        report.query_end_str = slices.back().range_label;

        // If any attribution query failed, report those bytes as Attribution Unavailable
        // rather than mislabeling them as OS kernel/network overhead.
        uint64_t total_error_d = 0, total_error_u = 0;
        for (size_t b = 0; b < slices.size(); ++b) {
            total_error_d += bucket_attr_error_down[b];
            total_error_u += bucket_attr_error_up[b];
        }
        if (total_error_d > 0 || total_error_u > 0) {
            AppUsageEntry attr_err;
            attr_err.id = L"__ATTR_FAILED__";
            attr_err.display_name = L"Attribution Unavailable (Query Error)";
            attr_err.detail = L"Windows SRUM attribution query failed for some time slices";
            attr_err.category = AppUsageCategory::Service;
            attr_err.download = total_error_d;
            attr_err.upload = total_error_u;
            attr_err.timeline.resize(slices.size());
            for (size_t b = 0; b < slices.size(); ++b) {
                attr_err.timeline[b].label = slices[b].label;
                attr_err.timeline[b].range_label = slices[b].range_label;
                attr_err.timeline[b].download = bucket_attr_error_down[b];
                attr_err.timeline[b].upload = bucket_attr_error_up[b];
            }
            aggregated.emplace(attr_err.id, std::move(attr_err));
        }

        // Capture genuinely unattributed traffic from successful queries
        // (kernel, hotspot tethering, VPN/TAP adapters, or untagged sockets).
        AppUsageEntry unattributed;
        unattributed.id = L"__OS_UNATTRIBUTED__";
        unattributed.display_name = L"Unattributed by Windows";
        unattributed.detail = L"Windows did not identify an app or service for these bytes";
        unattributed.category = AppUsageCategory::Service;
        unattributed.timeline.resize(slices.size());
        for (size_t b = 0; b < slices.size(); ++b) {
            unattributed.timeline[b].label = slices[b].label;
            unattributed.timeline[b].range_label = slices[b].range_label;
            const uint64_t valid_d = report.timeline[b].download > bucket_attr_error_down[b]
                ? report.timeline[b].download - bucket_attr_error_down[b] : 0;
            const uint64_t valid_u = report.timeline[b].upload > bucket_attr_error_up[b]
                ? report.timeline[b].upload - bucket_attr_error_up[b] : 0;

            const uint64_t res_d = valid_d > bucket_attr_down[b] ? valid_d - bucket_attr_down[b] : 0;
            const uint64_t res_u = valid_u > bucket_attr_up[b] ? valid_u - bucket_attr_up[b] : 0;
            unattributed.timeline[b].download = res_d;
            unattributed.timeline[b].upload = res_u;
            unattributed.download += res_d;
            unattributed.upload += res_u;
        }
        report.unattributed_download = unattributed.download;
        report.unattributed_upload = unattributed.upload;
        if (unattributed.Combined() > 0) {
            aggregated.emplace(unattributed.id, std::move(unattributed));
        }

        report.items.reserve(aggregated.size());
        for (auto& [key, entry] : aggregated) {
            uint64_t best = 0;
            size_t best_idx = 0;
            bool has_peak = false;
            for (size_t b = 0; b < entry.timeline.size(); ++b) {
                const uint64_t combined = entry.timeline[b].Combined();
                if (combined > best) {
                    best = combined;
                    best_idx = b;
                    has_peak = true;
                }
                if (combined > 0) {
                    entry.last_when = entry.timeline[b].range_label;
                }
            }
            if (has_peak) {
                entry.peak_when = entry.timeline[best_idx].range_label;
            }
            report.items.push_back(std::move(entry));
        }

        std::sort(report.items.begin(), report.items.end(), [](const AppUsageEntry& a, const AppUsageEntry& b) {
            if (a.Combined() != b.Combined()) return a.Combined() > b.Combined();
            return a.display_name < b.display_name;
        });

        report.active_services = GetActiveNetworkServices();
        report.updated_at = std::format(L"{:02d}:{:02d}:{:02d}", local_now.wHour, local_now.wMinute, local_now.wSecond);
        report.last_successful_update = report.updated_at;

        if (failed_slice_queries > 0 || any_attr_failed || !report.today_summary_valid || !report.month_summary_valid) {
            report.status = ReportStatus::Partial;
        } else {
            report.status = ReportStatus::Complete;
        }
        report.ready = true;
        out_report = std::move(report);
        return true;
    } catch (...) {}
    return false;
}

bool WindowsNetworkUsage::ExportDetailedCsv(const fs::path& path, const NetworkUsageReport& report) {
    if (!report.ready) return false;
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) return false;

    // 1. Write UTF-8 BOM so Microsoft Excel parses Unicode characters correctly
    file << "\xEF\xBB\xBF";

    const std::wstring range_name = [&]() -> std::wstring {
        switch (report.range) {
            case UsageTimeRange::LastHour: return L"Last 1 hour";
            case UsageTimeRange::Today: return L"Today";
            case UsageTimeRange::Last7Days: return L"Last 7 days";
            case UsageTimeRange::ThisMonth: return L"This month";
        }
        return L"Today";
    }();
    const std::wstring scope_name = report.scope == UsageNetworkScope::AllNetworks
        ? L"All networks"
        : (report.profile_label.empty() ? L"Active network" : report.profile_label);

    const char* status_str = (report.status == ReportStatus::Complete) ? "Complete" :
                             (report.status == ReportStatus::Partial) ? "Partial" :
                             (report.status == ReportStatus::Stale) ? "Stale" : "Unknown";

    // 2. Metadata comments for transparency and auditability
    file << "# Yash Traffic Monitor Pro - Detailed Network Usage Export\r\n";
    file << "# Generated: " << CsvEscape(report.updated_at.empty() ? L"N/A" : report.updated_at) << "\r\n";
    file << "# Range: " << CsvEscape(range_name) << "\r\n";
    file << "# Scope: " << CsvEscape(scope_name) << "\r\n";
    file << "# Report Status: " << status_str << "\r\n";
    file << "# Total Download Bytes: " << report.total_download << "\r\n";
    file << "# Total Upload Bytes: " << report.total_upload << "\r\n";
    file << "# Tracked Download Bytes: " << report.tracked_download << "\r\n";
    file << "# Tracked Upload Bytes: " << report.tracked_upload << "\r\n";
    file << "# Limitations: Windows SRUM records system-wide traffic across all sessions; VPN traffic may be counted on physical and virtual interfaces.\r\n";

    // 3. Header row
    file << "section,range,scope,name,detail,category,when_window,last_active,download_bytes,upload_bytes,combined_bytes\r\n";

    for (const auto& bucket : report.timeline) {
        if (bucket.Combined() == 0) continue;
        file << "overall_timeline,"
             << CsvEscape(range_name) << ','
             << CsvEscape(scope_name) << ','
             << "All Network Traffic,,"
             << "All,"
             << CsvEscape(bucket.range_label) << ','
             << CsvEscape(bucket.range_label) << ','
             << bucket.download << ','
             << bucket.upload << ','
             << bucket.Combined() << "\r\n";
    }

    for (const auto& item : report.items) {
        const std::wstring cat = item.category == AppUsageCategory::Service ? L"Service" : L"App";
        file << "app_summary,"
             << CsvEscape(range_name) << ','
             << CsvEscape(scope_name) << ','
             << CsvEscape(item.display_name) << ','
             << CsvEscape(item.detail) << ','
             << CsvEscape(cat) << ','
             << CsvEscape(item.peak_when) << ','
             << CsvEscape(item.last_when) << ','
             << item.download << ','
             << item.upload << ','
             << item.Combined() << "\r\n";
        for (const auto& bucket : item.timeline) {
            if (bucket.Combined() == 0) continue;
            file << "app_timeline,"
                 << CsvEscape(range_name) << ','
                 << CsvEscape(scope_name) << ','
                 << CsvEscape(item.display_name) << ','
                 << CsvEscape(item.detail) << ','
                 << CsvEscape(cat) << ','
                 << CsvEscape(bucket.range_label) << ','
                 << CsvEscape(item.last_when) << ','
                 << bucket.download << ','
                 << bucket.upload << ','
                 << bucket.Combined() << "\r\n";
        }
    }

    file.flush();
    return file.good();
}

} // namespace tmp
