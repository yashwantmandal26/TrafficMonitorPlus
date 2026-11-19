#pragma once
#include "pch.h"

namespace tmp {
enum class LiveNetworkState { Starting, Live, Unavailable, Stopped };
struct LiveNetworkProcess { DWORD pid = 0; uint64_t created = 0; };
struct LiveNetworkEntry {
    std::wstring id;
    std::wstring name;
    double download_bps = 0, upload_bps = 0;
    uint64_t today_download = 0, today_upload = 0;
    std::vector<LiveNetworkProcess> processes;
};
struct LiveNetworkSnapshot {
    LiveNetworkState state = LiveNetworkState::Starting;
    std::wstring message = L"Starting live app tracking...";
    std::wstring coverage_start;
    std::vector<LiveNetworkEntry> entries;
    uint64_t lost_events = 0;
    std::chrono::steady_clock::time_point sampled_at{};
};
}
