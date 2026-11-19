#pragma once
#include <string>
#include <string_view>
#include <cwctype>

namespace tmp {

enum class AppMode {
    NormalUI,
    SensorReport,
    ConfigureStartup,
    ExitRunning,
    RemoveStartup,
    Invalid
};

struct CommandLineOptions {
    AppMode mode = AppMode::NormalUI;
    bool show_settings = false;
    bool wait_for_instance = false;
    bool elevate_requested = false;
};

inline CommandLineOptions ParseCommandLine(std::wstring_view cmdLine) {
    CommandLineOptions options;
    // Trim leading whitespace
    while (!cmdLine.empty() && iswspace(cmdLine.front())) {
        cmdLine.remove_prefix(1);
    }
    // Trim trailing whitespace
    while (!cmdLine.empty() && iswspace(cmdLine.back())) {
        cmdLine.remove_suffix(1);
    }

    if (cmdLine.empty()) {
        return options;
    }

    if (cmdLine == L"--exit-running") {
        options.mode = AppMode::ExitRunning;
        return options;
    }

    if (cmdLine == L"--remove-startup") {
        options.mode = AppMode::RemoveStartup;
        return options;
    }
    if (cmdLine == L"--sensor-report") {
        options.mode = AppMode::SensorReport;
        return options;
    }

    if (cmdLine == L"--configure-startup") {
        options.mode = AppMode::ConfigureStartup;
        return options;
    }

    if (cmdLine == L"--show-settings") {
        options.mode = AppMode::NormalUI;
        options.show_settings = true;
        return options;
    }

    if (cmdLine == L"--wait-for-instance") {
        options.mode = AppMode::NormalUI;
        options.wait_for_instance = true;
        return options;
    }

    if (cmdLine == L"--elevate") {
        options.mode = AppMode::NormalUI;
        options.elevate_requested = true;
        return options;
    }

    // Tokenized scan for combinations (e.g. "--wait-for-instance --show-settings")
    size_t start = 0;
    bool recognized_any = false;
    while (start < cmdLine.size()) {
        while (start < cmdLine.size() && iswspace(cmdLine[start])) ++start;
        if (start >= cmdLine.size()) break;
        size_t end = start;
        while (end < cmdLine.size() && !iswspace(cmdLine[end])) ++end;
        std::wstring_view token = cmdLine.substr(start, end - start);
        start = end;

        if (token == L"--exit-running") {
            options.mode = AppMode::ExitRunning;
            recognized_any = true;
        } else if (token == L"--sensor-report") {
            options.mode = AppMode::SensorReport;
            recognized_any = true;
        } else if (token == L"--configure-startup") {
            options.mode = AppMode::ConfigureStartup;
            recognized_any = true;
        } else if (token == L"--show-settings") {
            options.show_settings = true;
            recognized_any = true;
        } else if (token == L"--wait-for-instance") {
            options.wait_for_instance = true;
            recognized_any = true;
        } else if (token == L"--elevate") {
            options.elevate_requested = true;
            recognized_any = true;
        } else {
            options.mode = AppMode::Invalid;
            return options;
        }
    }

    if (!recognized_any) {
        options.mode = AppMode::NormalUI;
    }
    return options;
}

inline std::wstring GetSingleInstanceMutexName(const std::wstring& userSid = {}) {
    if (!userSid.empty()) {
        return L"Local\\TrafficMonitorPlus_" + userSid;
    }
    DWORD sessionId = 0;
    ProcessIdToSessionId(GetCurrentProcessId(), &sessionId);
    return L"Local\\TrafficMonitorPlus_Session" + std::to_wstring(sessionId);
}

} // namespace tmp
