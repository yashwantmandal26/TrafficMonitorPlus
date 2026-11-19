// TrafficMonitorPlus - AutoStart Registry Management
#pragma once
#include "../pch.h"
#include <taskschd.h>

namespace tmp {

// Per-user interactive logon task, running with highest available privileges.
// Registration requires administrator access; no password is stored.
class AutoStart {
public:
    // Check whether the startup task is enabled.
    static bool IsEnabled();

    // Register elevated logon startup pointing to exe_path.
    static bool Enable(const std::wstring& exe_path);
    // Start this executable through its already-approved elevated task.
    static bool RunElevated();

    // Remove startup task and any legacy Run entry.
    static bool Disable();

    // Returns the full path of the currently running executable.
    static std::wstring GetCurrentExePath();

    // Check whether an existing Task Scheduler task matches the desired configuration and executable path.
    // Memory safe (RAII BSTR management, no double-free).
    static bool IsMatchingTask(IRegisteredTask* existing, const std::wstring& exe_path);

    // Returns the per-user scheduled task name.
    static std::wstring GetTaskName();

    // Returns the current user's string SID.
    static std::wstring GetCurrentUserSid();
};

} // namespace tmp
