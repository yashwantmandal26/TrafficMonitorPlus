// TrafficMonitorPlus - String Formatting Utilities
#pragma once
#include "../pch.h"

namespace tmp {

// Formats bytes/sec into human-readable speed: "0 B/s", "856 B/s", "12.8 KB/s", "1.4 MB/s", "2.3 GB/s"
std::wstring FormatSpeed(double bytes_per_sec);

// Formats bytes/sec in compact/short mode (TrafficMonitor classic: "0.3K/s", "12K/s", "1.4M/s")
std::wstring FormatSpeedCompact(double bytes_per_sec);

// Formats percentage: "24%" (no decimals, clamped to 0-100)
std::wstring FormatPercent(double percent);

// Formats temperature: "58°C"
std::wstring FormatTemperature(double celsius);

// Formats CPU/GPU frequency: "4.2 GHz"
std::wstring FormatFrequency(double ghz);

// Formats ping latency: "12ms" or "—" if negative (unavailable)
std::wstring FormatPing(double ms);

// Formats power draw: "45W" or "-12W"
std::wstring FormatWattage(double watts);

// Formats memory size from bytes: "15.4 GB"
std::wstring FormatMemory(uint64_t bytes);

// Gets the primary local IPv4 address. Returns "0.0.0.0" on failure.
std::wstring GetLocalIPAddress();

// Wide string (UTF-16) → UTF-8
std::string WideToUtf8(const std::wstring& wide);

// UTF-8 → Wide string (UTF-16)
std::wstring Utf8ToWide(const std::string& utf8);

} // namespace tmp
