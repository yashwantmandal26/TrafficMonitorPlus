// TrafficMonitorPlus - String Formatting Utilities
#include "pch.h"
#include "utils/string_utils.h"

#include <ws2tcpip.h>

namespace tmp {

std::wstring FormatSpeed(double bytes_per_sec) {
    if (!std::isfinite(bytes_per_sec)||bytes_per_sec<0)return L"--";

    if (bytes_per_sec < 1.0) {
        return L"0 B/s";
    }
    if (bytes_per_sec < 1024.0) {
        return std::format(L"{:.0f} B/s", bytes_per_sec);
    }

    double kb = bytes_per_sec / 1024.0;
    if (kb < 1024.0) {
        // Show 1 decimal for KB/s
        return std::format(L"{:.1f} KB/s", kb);
    }

    double mb = kb / 1024.0;
    if (mb < 1024.0) {
        return std::format(L"{:.1f} MB/s", mb);
    }

    double gb = mb / 1024.0;
    return std::format(L"{:.1f} GB/s", gb);
}

std::wstring FormatSpeedCompact(double bytes_per_sec) {
    if(!std::isfinite(bytes_per_sec)||bytes_per_sec<0)return L"--";
    double value=bytes_per_sec/1024.0;
    const wchar_t* units[]={L"K/s",L"M/s",L"G/s",L"T/s",L"P/s"};
    int unit=0;
    while(value>=1000&&unit<4){value/=1024.0;++unit;}
    return value<99.95?std::format(L"{:.1f}{}",value,units[unit]):std::format(L"{:.0f}{}",value,units[unit]);
}

std::wstring FormatPercent(double percent) {
    if (!std::isfinite(percent)||percent < 0.0) return L"--";
    return std::format(L"{}%",std::lround(std::clamp(percent,0.0,100.0)));
}

std::wstring FormatTemperature(double celsius) {
    if (!std::isfinite(celsius)||celsius < 0.0) return L"--";
    if(std::abs(celsius-std::round(celsius))>0.0001)return std::format(L"{:.1f}\u00B0C",celsius);
    return std::format(L"{:.0f}\u00B0C", celsius);
}

std::wstring FormatFrequency(double ghz) {
    if (!std::isfinite(ghz)||ghz < 0.0) return L"--";
    return std::format(L"{:.1f} GHz", ghz);
}

std::wstring FormatPing(double ms) {
    if (ms < 0.0) {
        return L"\u2014"; // em-dash for unavailable
    }
    return std::format(L"{:.0f}ms", ms);
}

std::wstring FormatWattage(double watts) {
    if (!std::isfinite(watts)) return L"--";
    return std::format(L"{:.1f}W", watts);
}

std::wstring FormatMemory(uint64_t bytes) {
    constexpr double GB = 1024.0 * 1024.0 * 1024.0;
    constexpr double MB = 1024.0 * 1024.0;
    constexpr double KB = 1024.0;

    double val = static_cast<double>(bytes);

    if (val >= GB) {
        return std::format(L"{:.1f} GB", val / GB);
    }
    if (val >= MB) {
        return std::format(L"{:.1f} MB", val / MB);
    }
    if (val >= KB) {
        return std::format(L"{:.1f} KB", val / KB);
    }
    return std::format(L"{} B", bytes);
}

std::wstring GetLocalIPAddress() {
    char hostname[256]{};
    if (::gethostname(hostname, sizeof(hostname)) != 0) {
        return L"0.0.0.0";
    }

    struct addrinfo hints{};
    hints.ai_family = AF_INET; // IPv4 only
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* result = nullptr;
    if (::getaddrinfo(hostname, nullptr, &hints, &result) != 0 || !result) {
        return L"0.0.0.0";
    }

    wchar_t ip_buf[INET_ADDRSTRLEN]{};
    bool found = false;

    for (auto* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        if (ptr->ai_family == AF_INET) {
            auto* addr_in = reinterpret_cast<struct sockaddr_in*>(ptr->ai_addr);
            if (::InetNtopW(AF_INET, &addr_in->sin_addr, ip_buf, INET_ADDRSTRLEN)) {
                found = true;
                break;
            }
        }
    }

    ::freeaddrinfo(result);
    return found ? std::wstring(ip_buf) : L"0.0.0.0";
}

std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return {};

    int size_needed = ::WideCharToMultiByte(
        CP_UTF8, 0,
        wide.data(), static_cast<int>(wide.size()),
        nullptr, 0,
        nullptr, nullptr);

    if (size_needed <= 0) return {};

    std::string utf8(static_cast<size_t>(size_needed), '\0');
    ::WideCharToMultiByte(
        CP_UTF8, 0,
        wide.data(), static_cast<int>(wide.size()),
        utf8.data(), size_needed,
        nullptr, nullptr);

    return utf8;
}

std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};

    int size_needed = ::MultiByteToWideChar(
        CP_UTF8, 0,
        utf8.data(), static_cast<int>(utf8.size()),
        nullptr, 0);

    if (size_needed <= 0) return {};

    std::wstring wide(static_cast<size_t>(size_needed), L'\0');
    ::MultiByteToWideChar(
        CP_UTF8, 0,
        utf8.data(), static_cast<int>(utf8.size()),
        wide.data(), size_needed);

    return wide;
}

} // namespace tmp
