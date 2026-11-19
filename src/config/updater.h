// TrafficMonitorPlus - Background GitHub Release Checker
#pragma once
#include "pch.h"
#include "types.h"
#include <thread>

namespace tmp {

struct UpdateInfo {
    bool         update_available = false;
    std::wstring latest_version;
    std::wstring release_notes;
    std::wstring download_url;
};

class Updater {
public:
    Updater() = default;
    ~Updater() { if (m_worker.joinable()) m_worker.join(); }

    /// Checks for updates asynchronously in a background worker thread.
    void CheckForUpdatesAsync(std::function<void(const UpdateInfo&)> on_result);

    /// Synchronous check (used in background thread)
    static UpdateInfo CheckForUpdatesSync();

    /// Version comparison helper (public for testing and validation)
    static bool IsNewerVersion(const std::wstring& remote, int local_major, int local_minor, int local_patch);

private:
    std::thread m_worker;
};

} // namespace tmp
